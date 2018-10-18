//This file contains code to lazily read event data from NanoAOD
//without having to predefine the full event structure in advance.
//Essentially, we create a collection of TTreeValueReader/TTreeArrayReader-s
//and access them from a map based on compile-time hashed strings.
//This means we can write code like event["Jet_pt"] without having to
//parse the string for every event, which would severely limit speed.
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderArray.h>
#include <TLeaf.h>
#include <TLeaf.h>
#include <ROOT/RDataFrame.hxx>

#include <typeinfo>

using namespace std;


//Compile-time hash function for strings from https://github.com/rioki/rex/blob/master/strex.h#L95
//By defining this, we can access members from the map by a string that is known at compile time
//without having to do runtime string hashing, meaning the code can be fast.
constexpr inline unsigned int string_hash(const char* str, int h = 0)
{
    return !str[h] ? 5381 : (string_hash(str, h+1)*33) ^ str[h];
}

inline unsigned int string_hash_cpp(const std::string& str)
{
    return string_hash(str.c_str());
} 


//Wraps arrays of a specific type from a TTree to TTreeReaderArray-s
template <typename T>
class LazyArrayReader {
public:
    TTreeReader& reader;

    unordered_map<unsigned int,  unique_ptr<TTreeReaderArray<T>>> reader_cache;
    //memory-contiguous data from the arrays
    unordered_map<unsigned int, ROOT::VecOps::RVec<T>> value_cache;

    LazyArrayReader(TTreeReader& _reader) : reader(_reader) {}

    //Creates the TTreeReaderArray for a specific branch on the heap and stores it in the cache
    void setup(const std::string& id) {
        const auto id_hash = string_hash_cpp(id);
        //cout << "Branch: vector " << typeid(T).name() << " " << id << endl;
        if(reader_cache.find(id_hash) == reader_cache.end()) {
            reader_cache[id_hash] = make_unique<TTreeReaderArray<T>>(reader, id.c_str());
        }
    }

    void read(const unsigned int& id_hash) {
        value_cache[id_hash] = ROOT::VecOps::RVec<T>(
            (*reader_cache.at(id_hash)).begin(),
            (*reader_cache.at(id_hash)).end()
        );
    }

    //Gets the value stored in a specific array at a specific index
    inline T get(const unsigned int& id_hash, unsigned int idx) const {

        //const TTreeReaderArray<T>& val = *reader_cache.at(id_hash);
        //return (*reader_cache.at(id_hash))[idx];
        return value_cache.at(id_hash)[idx];
    }
};

//Wraps numbers (values) of a specific type from a TTree to TTreeReaderValue-s
template <typename T>
class LazyValueReader {
public:
    unordered_map<unsigned int, unique_ptr<TTreeReaderValue<T>>> reader_cache;
    unordered_map<unsigned int, T> value_cache;
    TTreeReader& reader;

    LazyValueReader(TTreeReader& _reader) : reader(_reader) {}

    void setup(const std::string& id) {
        //cout << "Branch: " << typeid(T).name() << " " << id << endl;
        const auto id_hash = string_hash_cpp(id);
        if(reader_cache.find(id_hash) == reader_cache.end()) {
            reader_cache[id_hash] = make_unique<TTreeReaderValue<T>>(reader, id.c_str());
        }
    }

    void read(const unsigned int& id_hash) {
        value_cache[id_hash] = **reader_cache.at(id_hash);
    }

    inline T get(const unsigned int& id_hash) const {
        return value_cache.at(id_hash);
    }
};

//Wraps the full NanoAOD event with branches of different types
//to Array and Value readers automatically
class NanoEvent {
public:

    LazyArrayReader<Float_t> lc_vfloat;
    LazyArrayReader<Int_t> lc_vint;
    LazyArrayReader<UInt_t> lc_vuint;
    LazyArrayReader<Bool_t> lc_vbool;
    LazyArrayReader<UChar_t> lc_vuchar;

    LazyValueReader<Float_t> lc_float;
    LazyValueReader<Int_t> lc_int;
    LazyValueReader<UInt_t> lc_uint;
    LazyValueReader<Bool_t> lc_bool;
    LazyValueReader<UChar_t> lc_uchar;
    LazyValueReader<ULong64_t> lc_ulong64;

    // Connects all the existing branches from the TTree to Array and Value readers
    // Unless the readers are accessed, there is no overhead from this.
    NanoEvent(TTreeReader& reader) :
        lc_vfloat(reader),
        lc_vint(reader),
        lc_vuint(reader),
        lc_vbool(reader),
        lc_vuchar(reader),
        lc_float(reader), 
        lc_int(reader),
        lc_uint(reader),
        lc_bool(reader),
        lc_uchar(reader),
        lc_ulong64(reader) {

            for (auto leaf_obj : *reader.GetTree()->GetListOfLeaves()) {
            TLeaf* leaf = (TLeaf*)leaf_obj;
            const std::string dtype(leaf->GetTypeName());
            const std::string leaf_name(leaf->GetName());

            //cout << leaf_name << " " << dtype << endl;

            if (leaf->GetLeafCount() != nullptr) {
                if (dtype == "Float_t") {
                    lc_vfloat.setup(leaf_name);
                } else if (dtype == "Int_t") {
                    lc_vint.setup(leaf_name);
                } else if (dtype == "UInt_t") {
                    lc_vuint.setup(leaf_name);
                } else if (dtype == "Bool_t") {
                    lc_vbool.setup(leaf_name);
                } else if (dtype == "UChar_t") {
                    lc_vuchar.setup(leaf_name);
                } else {
                    cerr << "Could not understand array dtype " << dtype << " " << leaf_name << endl;
                }
            } else if (leaf->GetLeafCount() == nullptr) {
                if (dtype == "Float_t") {
                    lc_float.setup(leaf_name);
                } else if (dtype == "Int_t") {
                    lc_int.setup(leaf_name);
                } else if (dtype == "UInt_t") {
                    lc_uint.setup(leaf_name);
                } else if (dtype == "Bool_t") {
                    lc_bool.setup(leaf_name);
                } else if (dtype == "UChar_t") {
                    lc_uchar.setup(leaf_name);
                } else if (dtype == "ULong64_t") {
                    lc_ulong64.setup(leaf_name);
                } else {
                    cerr << "Could not understand number dtype " << dtype << " " << leaf_name << endl;
                }
            }
        }
    } // constructor

    virtual void analyze() = 0;
};

//Accesses data from the underlying TTree wrapped by a NanoEvent lazily
//using get_DTYPE(key) methods, e.g. get_float(string_hash("Jet_pt"))
//Each LazyObject contains an index, which corresponds to the index in the
//NanoAOD arrays.
class LazyObject {
public:

    //the reference to the parent NanoAOD event
    const NanoEvent& event;

    //the index of the object in the object array
    const unsigned int index;

    // unordered_map<unsigned int, float> userfloats;
    // mutable unordered_map<unsigned int, float> float_cache;

    //Initializes the object based on the event and the index
    LazyObject(const NanoEvent& _event, unsigned int _index) :
        event(_event), index(_index) { }
    virtual ~LazyObject() = 0;

    // Caching does not seem to be necessary
    // //Retrieves a float from the object 
    // Float_t get_float(const unsigned int string_hash) const {
    //     const auto& cache_key = float_cache.find(string_hash);
    //     if (cache_key == float_cache.end()) {
    //         const auto& v = event.lc_vfloat.get(string_hash, index);
    //         float_cache[string_hash] = v;
    //         return v;
    //     }
    //     return cache_key->second;
    // }

    //Retrieves a float from the object 
    Float_t get_float(const unsigned int string_hash) const {
        return event.lc_vfloat.get(string_hash, index);
    }

    //Retrieves an int from the object
    Int_t get_int(const unsigned int string_hash) const {
        return event.lc_vint.get(string_hash, index);
    }

    virtual float pt() const = 0;
    virtual float eta() const = 0;
    virtual float phi() const = 0;
    virtual float mass() const = 0;
    //Add here getters for other datatypes
};


//This is here to verify the string hashing at compile time
static_assert(string_hash("Jet_pt") == 1724548869, "compile-time string hashing failed");
