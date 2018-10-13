#include "myanalyzers.h"

//This is the main event loop
//Given a TTreeReader reader, we process all the specified analyzers and store the
//output in the Output data structure.
//You shouldn't have to add anything to the event loop if you want to compute a new
//quantity - rather, you can add a new Analyzer
FileReport looper_main(
    const string& filename,
    TTreeReader& reader,
    Output& output,
    const vector<Analyzer*>& analyzers,
    long long max_events
    ) {

    //Make sure we clear the state of the reader
    reader.Restart();
    
    TStopwatch sw;
    sw.Start();

    //We initialize the event from the TTreeReader
    Event event(reader);

    //Keep track of the number of events we processed
    unsigned long long nevents = 0;

    //Keep track of the total time per event
    FileReport report(filename, analyzers);

    //This is the main event loop
    cout << "starting loop over " << reader.GetEntries(true) << " events in TTree " << reader.GetTree() << endl;
    while (reader.Next()) {

        //In case of early termination
        if (max_events > 0 && nevents == max_events) {
            cout << "breaking event loop before event " << nevents << endl;
            break;
        }

        auto time_t0 = std::chrono::high_resolution_clock::now();

        //We create the event from the NanoAOD data
        //This reads some data from the ROOT file, but not the whole file
        event.analyze();

        auto time_t1 = std::chrono::high_resolution_clock::now();
        auto time_dt = std::chrono::duration_cast<std::chrono::nanoseconds>(time_t1 - time_t0).count();
        report.event_duration += time_dt;

        unsigned int iAnalyzer = 0;

        //We run all the analyzers one after the other
        for (auto* analyzer : analyzers) {

            auto time_t0 = std::chrono::high_resolution_clock::now();

            //Here we do the actual work for the analyzer
            analyzer->analyze(event);

            //Get the time in nanoseconds spent per event for this analyzer
            auto time_t1 = std::chrono::high_resolution_clock::now();
            auto time_dt = std::chrono::duration_cast<std::chrono::nanoseconds>(time_t1 - time_t0).count();
            report.analyzer_durations[iAnalyzer] += time_dt;

            iAnalyzer += 1;
        }

        //Print out a progress report
        if (nevents % 50000 == 0) {
            const auto elapsed_time = sw.RealTime();
            cout << "Processed " << nevents << "/" << reader.GetEntries(true) << " speed=" << nevents/elapsed_time/1000.0 << "kHz" << endl;
            sw.Continue();
        }
        nevents += 1;
    }
    report.num_events_processed = nevents;

    sw.Stop();

    //Print out some statistics
    report.cpu_time = sw.CpuTime();
    report.real_time = sw.RealTime();

    //Compute the event processing speed in kHz
    report.speed = (double)report.num_events_processed / report.real_time / 1000.0;
    
    report.print(cout);

    cout << "looper_main" << " nevents=" << report.num_events_processed << ",cpu_time=" << report.cpu_time << ",real_time=" << report.real_time << ",speed=" << report.speed << endl;

    return report;
}

TLorentzVector make_lv(float pt, float eta, float phi, float mass) {
    TLorentzVector lv;
    lv.SetPtEtaPhiM(pt, eta, phi, mass);
    return lv;
}
