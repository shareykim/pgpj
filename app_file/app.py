# app.py
from flask import Flask, render_template, request, jsonify
import requests
import subprocess
import json
import os

app = Flask(__name__)

# 네이버 API 클라이언트 ID와 시크릿키
NAVER_CLIENT_ID = '5oJtUmA9ooPzMqK8qLR4'
NAVER_CLIENT_SECRET = 'y5gbJdi6rA'
results = []

# 루트 경로('/')에 대한 라우트 설정 (템플릿을 연동하거나 사용자 요청을 처리시키는데에 필요함)
@app.route('/', methods=['GET', 'POST'])
def index():
    global results
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
                with open('results.json', 'w', encoding='utf-8') as f:
                    json.dump(results, f, ensure_ascii=False, indent=4)
            else:
                print('Error:', response.status_code)  # 오류 발생 시 상태 코드 출력
    # index.html 템플릿에 검색 결과를 넘겨줌
    return render_template('index.html', results=results)

@app.route('/find_route', methods=['POST'])
def find_route():
    try:
        # 현재 디렉토리를 기준으로 파일 경로 설정
        base_dir = os.path.dirname(os.path.abspath(__file__))  # app.py의 절대 경로
        distance_find_path = os.path.join(base_dir, '../distance_find.py')  # 상위 폴더의 distance_find.py
        dijkstra_path = os.path.join(base_dir, '../dijkstra.c')  # 상위 폴더의 dijkstra.c

        # 거리 계산 파일 실행 (distance_find.py)
        #subprocess.run(['python', distance_find_path], check=True)
        
        # distance_find.c 컴파일 및 실행
        subprocess.run(['gcc', '-o', 'distance_find', distance_find_path, 'parson.c'], check=True)
        subprocess.run(['./distance_find'], check=True, cwd=os.path.join(base_dir, '../'))  # 실행 위치를 상위 폴더로 설정

        # dijkstra.c 컴파일 및 실행
        subprocess.run(['gcc', '-o', 'dijkstra', dijkstra_path, 'parson.c'], check=True)
        subprocess.run(['./dijkstra'], check=True, cwd=os.path.join(base_dir, '../'))  # 실행 위치를 상위 폴더로 설정

        optimal_path_file = os.path.join(base_dir, '../최적의_경로.json')
        with open(optimal_path_file, 'r') as f:
            path_data = json.load(f)

        # 최적 경로 반환
        return jsonify({'status': 'success', 'path': path_data})

    except subprocess.CalledProcessError as e:
        print("Error during execution:", e)
        return jsonify({'status': 'error', 'message': str(e)})


# Flask 애플리케이션 실행 (디버그 모드와 포트 설정 포함)
if __name__ == '__main__':
    app.run(debug=True, port=5001)
