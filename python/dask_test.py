import dask
from dask.distributed import Client
client = Client("10.3.11.130:8786")
print client

ret = client.map(lambda x: x**2, range(10))
tot = client.submit(sum, ret)
print tot.result()
