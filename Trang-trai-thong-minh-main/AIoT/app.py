from flask import Flask, render_template, request, jsonify, Response
from flask_socketio import SocketIO
import json
import cv2

app = Flask(__name__)
socketio = SocketIO(app)

sensor_data = {"uno1": "Chưa có dữ liệu", "uno2": "Chưa có dữ liệu", "uno3": "Chưa có dữ liệu"}
ESP32_CAM_URL = "http://192.168.1.46:81/stream"
latest_command = "" 

def generate_frames():
    cap = cv2.VideoCapture(ESP32_CAM_URL)
    while True:
        success, frame = cap.read()
        if not success:
            break
        else:
            ret, buffer = cv2.imencode('.jpg', frame)
            frame = buffer.tobytes()
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')

@app.route('/video_feed')
def video_feed():
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/update', methods=['POST'])
def update_data():
    try:
        data = request.get_json(force=True)
        if not data:
            print("❌ Không nhận được dữ liệu JSON!")
            return jsonify({"error": "Invalid JSON"}), 400

        print("📥 Dữ liệu nhận từ ESP32:", json.dumps(data, indent=2, ensure_ascii=False))

        global sensor_data
        sensor_data = data
        socketio.emit('sensor_update', sensor_data)
        return jsonify({"status": "success"})
    
    except Exception as e:
        print("❌ Lỗi xử lý JSON:", str(e))
        return jsonify({"error": str(e)}), 400

@app.route('/control', methods=['GET'])
def get_control_command():
    global latest_command
    return Response(latest_command, mimetype='text/plain')

@socketio.on('control')
def handle_control(data):
    global latest_command
    command = data['command']
    print(f"📡 Nhận lệnh từ web: {command}")
    latest_command = command
    socketio.emit('control_command', command)

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)