from flask import Flask, request, jsonify
from flasgger import Swagger
import os
import random

app = Flask(__name__)

swagger = Swagger(app, template={
    "swagger": "2.0",
    "info": {
        "title": "Lab12",
        "description": "API reading ESP32 sensors",
        "version": "1.0.0",
        "contact": {
            "name": "Rares Leonte",
            "email": "rares.leonte@student.tuiasi.ro"
        }
    },
    "basePath": "/",
    "schemes": ["http"]
})

SENSORS_DIR = 'sensors'
os.makedirs(SENSORS_DIR, exist_ok=True)

def get_sensor_value(sensor_id):
    random.seed(sensor_id)
    return round(random.uniform(10.0, 100.0), 2)

def config_path(sensor_id, filename='config.json'):
    sensor_dir = os.path.join(SENSORS_DIR, sensor_id)
    os.makedirs(sensor_dir, exist_ok=True)
    return os.path.join(sensor_dir, filename)

@app.route('/sensor/<sensor_id>', methods=['GET'])
def read_sensor(sensor_id):
    """
    Get current value from sensor
    ---
    parameters:
      - name: sensor_id
        in: path
        type: string
        required: true
        description: ID of the sensor to read
    responses:
      200:
        description: Current value from sensor
        schema:
          type: object
          properties:
            sensor_id:
              type: string
            value:
              type: number
    """
    value = get_sensor_value(sensor_id)
    return jsonify({'sensor_id': sensor_id, 'value': value})

@app.route('/sensor/<sensor_id>/config', methods=['POST'])
def create_config(sensor_id):
    """
    Create configuration file for a sensor
    ---
    parameters:
      - name: sensor_id
        in: path
        type: string
        required: true
      - name: body
        in: body
        required: false
        schema:
          type: object
          properties:
            scale:
              type: string
            frequency:
              type: string
    responses:
      201:
        description: Configuration file created
      409:
        description: Configuration already exists
    """
    path = config_path(sensor_id)
    if os.path.exists(path):
        return jsonify({'error': 'Configuration already exists'}), 409

    data = request.get_json() or {}
    config = {
        "scale": data.get("scale", "Celsius"),
        "frequency": data.get("frequency", "1Hz")
    }

    with open(path, 'w') as f:
        f.write(str(config))

    return jsonify({'message': 'Configuration created', 'config': config}), 201

@app.route('/sensor/<sensor_id>/config/<filename>', methods=['PUT'])
def update_config(sensor_id, filename):
    """
    Update configuration file for a sensor
    ---
    parameters:
      - name: sensor_id
        in: path
        type: string
        required: true
      - name: filename
        in: path
        type: string
        required: true
      - name: body
        in: body
        required: true
        schema:
          type: object
          properties:
            scale:
              type: string
            frequency:
              type: string
    responses:
      200:
        description: Configuration updated
      404:
        description: File does not exist
    """
    path = config_path(sensor_id, filename)
    if not os.path.exists(path):
        return jsonify({'error': 'Configuration file does not exist'}), 404

    data = request.get_json()
    if not data:
        return jsonify({'error': 'Missing config data'}), 400

    with open(path, 'w') as f:
        f.write(str(data))

    return jsonify({'message': 'Configuration updated', 'config': data})

@app.route('/')
def index():
    return '<h3>Go to <a href="/apidocs/">Swagger UI</a> to test the API</h3>'

if __name__ == '__main__':
    app.run(debug=True)
