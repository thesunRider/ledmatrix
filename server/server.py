from flask import Flask, render_template, request, jsonify
import os

panel_update = 50 
panel_width = 64
panel_height = 32
panel_count = 2

app = Flask(__name__)

# Folder to store uploaded images
UPLOAD_FOLDER = "img"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

@app.route("/")
def index():
	return render_template("index.html")

@app.route("/settings", methods=["POST"])
def upload_text():
	text = request.form.get("text", "Welcome to Elab!")
	size = request.form.get("size", "120")
	color = request.form.get("color", "#ffffff")
	print(text)
	return ""

@app.route("/upload-text", methods=["POST"])
def upload_text():
	text = request.form.get("text", "Welcome to Elab!")
	size = request.form.get("size", "120")
	color = request.form.get("color", "#ffffff")
	print(text)
	return ""

@app.route("/upload-image", methods=["POST"])
def upload_image():
	file = request.files.get("image")

	if not file:
		return jsonify({"status": "error", "msg": "No file uploaded"}), 400

	filepath = os.path.join(UPLOAD_FOLDER, file.filename)
	file.save(filepath)

	return jsonify({
		"status": "ok",
		"msg": "Image saved",
		"path": filepath
	})

if __name__ == "__main__":
	app.run(debug=True)
