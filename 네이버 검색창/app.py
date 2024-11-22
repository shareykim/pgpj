# app.py
from flask import Flask, render_template, request
import requests

app = Flask(__name__)
results = [] # 검색 결과를 저장할 리스트

# 네이버 API 클라이언트 ID와 시크릿키
NAVER_CLIENT_ID = '5oJtUmA9ooPzMqK8qLR4'
NAVER_CLIENT_SECRET = 'y5gbJdi6rA'

# 루트 경로('/')에 대한 라우트 설정 (템플릿을 연동하거나 사용자 요청을 처리시키는데에 필요함)
@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':  # 사용자가 POST 요청을 했을 때만 실행
        query = request.form.get('query')  # 사용자가 입력한 검색어를 가져옴
        if query:  # 검색어가 비어있지 않은 경우에만 실행
            headers = {  # 네이버 API 요청에 필요한 헤더 설정
                'X-Naver-Client-Id': NAVER_CLIENT_ID,
                'X-Naver-Client-Secret': NAVER_CLIENT_SECRET
            }
            params = {  # 요청에 필요한 파라미터 설정
                'query': f'강릉 {query}',  # "강릉"과 사용자가 입력한 검색어를 조합하여 검색
                'display': 5,  # 검색 결과를 5개로 제한
                'start': 1,  # 검색 시작 위치
                'sort': 'random'  # 검색 결과를 무작위로 정렬
            }
            # 네이버 로컬 검색 API에 GET 요청을 보냄
            response = requests.get('https://openapi.naver.com/v1/search/local.json', headers=headers, params=params)
            if response.status_code == 200:  # 응답이 성공적일 때
                results = response.json().get('items', [])  # 결과에서 'items' 키의 값을 가져옴
            else:
                print('Error:', response.status_code)  # 오류 발생 시 상태 코드 출력
    # index.html 템플릿에 검색 결과를 넘겨줌
    return render_template('index.html', results=results)

# Flask 애플리케이션 실행 (디버그 모드와 포트 설정 포함)
if __name__ == '__main__':
    app.run(debug=True, port=5001)