from flask import Flask, Response, render_template, url_for

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


with app.test_request_context():
    print(url_for('index'))
    print(url_for('static', filename='style.css'))
    print(url_for('static', filename='plots/plot.png'))


if __name__ == '__main__':
    app.run()
