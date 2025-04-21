import pandas as pd
import json


with open('test.json', 'r') as f: 
    data = json.load(f)
print(type(data), len(data))

