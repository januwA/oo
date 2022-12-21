-- print("method:", request["method"])
-- print("url:", request["url"])
-- print("requestCount:", request["requestCount"])

-- print("data:", request.data)

-- for k, v in pairs(request.multipart) do
-- 	print(k, v.name, v.isFilePath, v.values[0])
-- end

-- for k, v in pairs(request.headers) do
-- 	print(k, v)
-- end

function Preset()
	request.method = 'get'
	-- request.data = '{"data": "json"}'
	-- request.headers['content-type'] = 'application/json'
	-- request.headers['X-name'] = 'ajanuw'
	request.url = "http://localhost:7777/ping"
end

function Response(response)
	-- print(response.size)
	-- print(response.statusCode)
	-- print(response.headers)
	-- print(response.body.size)
	-- print(response.body.text())
	-- print(response.body.json().name)
	return true
end