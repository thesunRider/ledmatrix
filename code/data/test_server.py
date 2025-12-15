from http.server import HTTPServer, SimpleHTTPRequestHandler
import os
import shutil

UPLOAD_DIR = "uploads"
os.makedirs(UPLOAD_DIR, exist_ok=True)

class MyHandler(SimpleHTTPRequestHandler):

    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        content_type = self.headers.get('Content-Type', '')
        
        if 'multipart/form-data' in content_type:
            boundary = content_type.split("boundary=")[1].encode()
            body = self.rfile.read(content_length)

            parts = body.split(b"--" + boundary)
            for part in parts:
                if b'Content-Disposition' in part and b'name="image"' in part:
                    # Extract file content after empty line
                    file_data = part.split(b"\r\n\r\n", 1)[1].rsplit(b"\r\n", 1)[0]
                    filename = "capture.bmp"  # Or generate unique name
                    filepath = os.path.join(UPLOAD_DIR, filename)
                    with open(filepath, 'wb') as f:
                        f.write(file_data)
                    print(f"Saved file to {filepath}")
                    break

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"msg":"File saved"}')
        else:
            self.send_response(400)
            self.end_headers()
            self.wfile.write(b'{"msg":"Invalid request"}')


PORT = 80
httpd = HTTPServer(("", PORT), MyHandler)
print(f"Serving at port {PORT}")
httpd.serve_forever()
