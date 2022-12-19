-- request 请求的配置，如果提前在命令行已设置

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
	request.method = 'post'
	-- request.data = '{"data": "json"}'
	-- request.headers['content-type'] = 'application/json'
	-- request.headers['X-name'] = 'ajanuw'
	request.url = "http://localhost:7777/ping"
end

function Response(response)
	-- print(response.body.size)
	print(response.body.text())
	return true
end