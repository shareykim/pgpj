# app.py (Flask server-side code)
from flask import Flask, render_template, request
import requests

app = Flask(__name__)

NAVER_CLIENT_ID = '5oJtUmA9ooPzMqK8qLR4'
NAVER_CLIENT_SECRET = 'y5gbJdi6rA'

@app.route('/', methods=['GET', 'POST'])
def index():
    results = []
    if request.method == 'POST':
        query = request.form.get('query')
        if query:
            headers = {
                'X-Naver-Client-Id': NAVER_CLIENT_ID,
                'X-Naver-Client-Secret': NAVER_CLIENT_SECRET
            }
            params = {
                'query': f'강릉 {query}',
                'display': 5,
                'start': 1,
                'sort': 'random'
            }
            response = requests.get('https://openapi.naver.com/v1/search/local.json', headers=headers, params=params)
            if response.status_code == 200:
                results = response.json().get('items', [])
            else:
                print('Error:', response.status_code)
    return render_template('index.html', results=results)

if __name__ == '__main__':
    app.run(debug=True, port=5001)