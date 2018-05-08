import json
import sys
import itertools

json_data = json.load(sys.stdin)

def lisp_dump(data):
    if isinstance(data, int):
        return str(data)
    elif isinstance(data, float):
        return str(data) 
    elif isinstance(data, str):
        escaped_string = data.encode("unicode_escape").decode("utf-8")
        return "\"" + escaped_string + "\""
    elif isinstance(data, list):
        result = "("
        for i, item in enumerate(data):
            result += lisp_dump(item)
            if i + 1 < len(data):
                result += " "
        result += ")"
        return result
    elif isinstance(data, dict):
        result = "#("
        i = 0
        for key, item in data.items():
            result += "(%s . %s)" % (key, lisp_dump(item))
            if i + 1 < len(data):
                result += " "
            i += 1
        result += ")"
        return result
    else:
        print("error")
        print(type(data))

print(lisp_dump(json_data))

