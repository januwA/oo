-- command set -df or -dF
-- for k, v in pairs(request.multipart) do
-- 	print(k, v.name, v.isFilePath, v.values[0])
-- end

function Preset()
	request.method = 'post'

	request.multipart = {}
	request.multipart[0] = { name = "foo", isFilePath = false, values = { "bar" } };
	request.multipart[1] = { name = "file", isFilePath = true, values = { "/media/a.jpg" } };

	request.url = "http://localhost:7777/api/upload/local"
end

function Response(response)
	return response.statusCode == 200 and response.body.json().code == 0
end
