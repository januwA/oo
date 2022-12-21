-- request 请求的配置，如果提前在命令行已设置
-- print("method:", request["method"])
-- print("url:", request["url"])
-- print("requestCount:", request["requestCount"])

-- for k, v in pairs(request.headers) do
-- 	print(k, v)
-- end

-- 在发送请求前，设置一些参数
function Preset()
	print("Preset 执行一次")
	request.method = "get"
	-- request.requestCount = 20
	request.url = "http://localhost:7777/ping"
end

-- 每个请求发送成功，收到响应后都会调用这个函数，返回bool来表示请求成功还是失败
-- 如果请求直接失败则不会调用
function Response(response)
	print("Response 每个请求执行一次")
	print("返回状态码:", response.statusCode)

	for k, v in pairs(response.headers) do
		print("返回的header: " .. k .. ":" .. v)
	end

	return response.statusCode == 200
end

-- 运行结束，自定义打印
function RunDone(result)
	print("RunDone 执行一次")
	print("运行结束: " .. result.timeMs .. "ms")
	print("线程数:", result.threadCount)
	print("返回字节数:", result.respDataCount)
	print("成功请求数:", result.successCount)
	print("失败请求数:", result.errorCount)
end
