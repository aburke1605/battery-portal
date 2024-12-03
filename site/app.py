from flask import Flask, Response, render_template, url_for, request

import random
import matplotlib.pyplot as plt

app = Flask(__name__)

# this would be done in some other script
# whether that be on a regular basis or something?
fig, ax = plt.subplots()
xs = range(100)
ys = [random.randint(1, 50) for x in xs]
ax.plot(xs, ys)
plt.savefig('static/plots/plot.png')

@app.route('/')
def index():
    print('Request for index page received')
    return render_template('index.html')

data_store = []

@app.route('/data', methods=['POST'])
def receive_data():
    data = request.json  # Expecting JSON payload
    if data:
        data_store.append(data)  # Store or process data as needed
        print(f"Received: {data}")
        return {"status": "success", "data_received": data}, 200
    return {"status": "error", "message": "No data received"}, 400


with app.test_request_context():
    print(url_for('index'))
    print(url_for('static', filename='style.css'))
    print(url_for('static', filename='plots/plot.png'))


if __name__ == '__main__':
    app.run()