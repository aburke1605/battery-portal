from flask import Flask, Response, render_template, url_for

import io
import random
from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
import matplotlib.pyplot as plt

app = Flask(__name__)

@app.route('/')
def index():
   print('Request for index page received')
   return render_template('index.html')


def create_figure():
    fig, ax = plt.subplots()
    xs = range(100)
    ys = [random.randint(1, 50) for x in xs]
    ax.plot(xs, ys)
    return fig
@app.route('/plot.png')
def plot_png():
    fig = create_figure()
    output = io.BytesIO()
    FigureCanvas(fig).print_png(output)
    return Response(output.getvalue(), mimetype='image/png')


with app.test_request_context():
    print(url_for('static', filename='style.css'))
    print(url_for('index'))
    print(url_for('plot_png'))


if __name__ == '__main__':
   app.run()
