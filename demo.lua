print("method:", request["method"])
print("url:", request["url"])
print("requestCount:", request["requestCount"])

for k,v in pairs(request["headers"]) do
  print(k, v)
end