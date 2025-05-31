from flask import Flask, request, jsonify
from flasgger import Swagger
import os
import uuid

app = Flask(__name__)

swagger = Swagger(app, template={
    "swagger": "2.0",
    "info": {
        "title": "Lab11",
        "description": "API for managing text files on the server (CRUD operations)",
        "version": "1.0.0",
        "contact": {
            "name": "Rares Leonte",
            "email": "rares.leonte@student.tuiasi.ro"
        }
    },
    "basePath": "/",
    "schemes": ["http"]
})

FILES_DIR = 'files'
os.makedirs(FILES_DIR, exist_ok=True)

def file_path(filename):
    return os.path.join(FILES_DIR, filename)

@app.route('/files', methods=['GET'])
def list_files():
    """
    Returns a list of all files stored on the server.
    ---
    responses:
      200:
        description: A list of filenames
        schema:
          type: array
          items:
            type: string
    """
    return jsonify(os.listdir(FILES_DIR))

@app.route('/files/<filename>', methods=['GET'])
def get_file_content(filename):
    """
    Get the content of a file
    ---
    parameters:
      - name: filename
        in: path
        type: string
        required: true
        description: Name of the file to retrieve
    responses:
      200:
        description: File content retrieved successfully
        schema:
          type: object
          properties:
            content:
              type: string
      404:
        description: File not found
    """
    path = file_path(filename)
    if not os.path.isfile(path):
        return jsonify({'error': 'File not found'}), 404
    with open(path, 'r') as f:
        return jsonify({'content': f.read()})

@app.route('/files', methods=['POST'])
def create_file():
    """
    Create a file with a given name and content
    ---
    parameters:
      - name: body
        in: body
        required: true
        schema:
          type: object
          required: [content]
          properties:
            filename:
              type: string
              description: Optional name for the file
            content:
              type: string
              description: Text content to write into the file
    responses:
      201:
        description: File created successfully
        schema:
          type: object
          properties:
            message:
              type: string
            filename:
              type: string
      400:
        description: Invalid input
    """
    data = request.get_json()
    if not data:
        return jsonify({'error': 'Missing JSON body'}), 400
    content = data.get('content', '')
    filename = data.get('filename', str(uuid.uuid4()) + '.txt')
    path = file_path(filename)
    with open(path, 'w') as f:
        f.write(content)
    return jsonify({'message': 'File created', 'filename': filename}), 201

@app.route('/files/<filename>', methods=['DELETE'])
def delete_file(filename):
    """
    Delete a file
    ---
    parameters:
      - name: filename
        in: path
        type: string
        required: true
        description: Name of the file to delete
    responses:
      200:
        description: File deleted successfully
      404:
        description: File not found
    """
    path = file_path(filename)
    if not os.path.isfile(path):
        return jsonify({'error': 'File not found'}), 404
    os.remove(path)
    return jsonify({'message': 'File deleted'})

@app.route('/files/<filename>', methods=['PUT'])
def update_file(filename):
    """
    Update the content of a file
    ---
    parameters:
      - name: filename
        in: path
        type: string
        required: true
        description: Name of the file to update
      - name: body
        in: body
        required: true
        schema:
          type: object
          required: [content]
          properties:
            content:
              type: string
              description: New content to write into the file
    responses:
      200:
        description: File updated successfully
      404:
        description: File not found
    """
    path = file_path(filename)
    if not os.path.isfile(path):
        return jsonify({'error': 'File not found'}), 404

    data = request.get_json()
    content = data.get('content', '')
    with open(path, 'w') as f:
        f.write(content)
    return jsonify({'message': 'File updated'})

@app.route('/')
def index():
    return '<h3>Go to <a href="/apidocs/">Swagger UI</a> to test the API</h3>'

if __name__ == '__main__':
    app.run(debug=True)
