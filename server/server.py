from flask import Flask, render_template, request, jsonify
import os
from PIL import Image

panel_update = 50 
panel_width = 64
panel_height = 32
panel_count = 2

# 0 - for graphic
# 1 - for text
# 2 - for animation
USE_GRAPHIC = 0
USE_TEXT = 1
USE_ANIMATION = 2
method_graphics = USE_TEXT


graphic_text = "Welcome to Elab!"
graphic_size = 120
graphic_color = "#ffffff"

app = Flask(__name__)

# Folder to store uploaded images
UPLOAD_FOLDER = "img"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs("anim", exist_ok=True)

#ALL API FROM WEBEND BELOW
@app.route("/")
def index():
	return render_template("index.html")

@app.route("/settings", methods=["POST"])
def settings():
	global panel_update,panel_width,panel_height,panel_count
	panel_update = request.form.get("text", "50")
	panel_width = request.form.get("size", "64")
	panel_height = request.form.get("color", "32")
	panel_count = request.form.get("color", "2")
	return ""

@app.route("/upload-anim", methods=["POST"])
def upload_anim():
	global method_graphics
	method_graphics = USE_ANIMATION
	file = request.files.get("anim")

	if not file:
		return jsonify({"status": "error", "msg": "No file uploaded"}), 400

	filepath = os.path.join("anim", "anim.zip")
	file.save(filepath)

	return jsonify({
		"status": "ok",
		"msg": "Uploaded Animation",
		"path": filepath
	})
	return ""

@app.route("/upload-text", methods=["POST"])
def upload_text():
	global graphic_text,graphic_size,graphic_color,method_graphics
	method_graphics = USE_TEXT
	graphic_text = request.form.get("text", "Welcome to Elab!")
	graphic_size = request.form.get("size", "120")
	graphic_color = request.form.get("color", "#ffffff")
	print(graphic_text)
	return ""

@app.route("/upload-image", methods=["POST"])
def upload_image():
	global method_graphics
	method_graphics = USE_GRAPHIC
	file = request.files.get("image")

	if not file:
		return jsonify({"status": "error", "msg": "No file uploaded"}), 400

	filepath = os.path.join(UPLOAD_FOLDER, "capture.png")
	file.save(filepath)

	return jsonify({
		"status": "ok",
		"msg": "Image saved",
		"path": filepath
	})


#ALL API FROM ESP32 BELOW


if __name__ == "__main__":
	app.run(debug=True)