# app.py
# 카테고리 관련 코드 모두 최은지 작성
from flask import Flask, render_template, request, jsonify, session, redirect, url_for
import requests
import subprocess
import json
import os
import signal
import sys

app = Flask(__name__)
app.secret_key = os.environ.get('FLASK_SECRET_KEY', 'your_random_secret_key')

# 네이버 API 클라이언트 ID와 시크릿키
NAVER_CLIENT_ID = '5oJtUmA9ooPzMqK8qLR4'
NAVER_CLIENT_SECRET = 'y5gbJdi6rA'
results = []


# JSON 파일 초기화 및 세션 초기화 함수
def resetall(): #최은지
    # Flask 세션 초기화 (리셋)
    session.clear()
    
    # JSON 파일 초기화
    with open('results.json', 'w', encoding='utf-8') as f:
        pass
    with open('distance.json', 'w', encoding='utf-8') as f:
        pass
    with open('route.json', 'w', encoding='utf-8') as f:
        pass

    # 'selectedPlaces'를 빈 리스트로 세션에 저장
    session['selectedPlaces'] = []


def on_exit(signum, frame):
    print(f"Received signal {signum}. Initializing results.json...")
    base_dir = os.path.dirname(os.path.abspath(__file__))
    results_file = os.path.join(base_dir, 'results.json')
    
    # results.json 파일 초기화
    with open(results_file, 'w', encoding='utf-8') as f:
        json.dump([], f, ensure_ascii=False, indent=4)
    
    print("results.json has been initialized.")
    sys.exit(0)

# 신호 처리기 등록
signal.signal(signal.SIGINT, on_exit)   # Ctrl+C
signal.signal(signal.SIGTERM, on_exit)  # Termination signal

# 서버 시작 시 results.json 초기화 (필요 시)
def initialize_results_json():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    results_file = os.path.join(base_dir, 'results.json')
    if os.path.exists(results_file):
        os.remove(results_file)
    # 빈 리스트로 초기화
    with open(results_file, 'w', encoding='utf-8') as f:
        json.dump([], f, ensure_ascii=False, indent=4)

# 애플리케이션 시작 시 초기화 실행
initialize_results_json()

# 홈 화면 라우트
@app.route('/', methods=['GET']) #최은지
def home():
    return render_template('home.html')

# 루트 경로('/')에 대한 라우트 설정 (송유진)
@app.route('/indexnext', methods=['GET', 'POST'])
def indexnext():
    global results
    if request.method == 'GET': #최은지
        resetall()  # 홈 화면에서 넘어올 때 초기화
        results = []
    if request.method == 'POST':  # 사용자가 POST 요청을 했을 때만 실행
        # 1. 기존 선택된 장소와 카테고리 정보 저장
        selected_places = session.get('selectedPlaces', [])  # 현재까지 선택된 장소를 가져옴

        # 2. 카테고리 값만 업데이트 (주소별로)
        for place in selected_places:
            category_key = f"category_{place['name']}"  # 각 항목의 category에 해당하는 키를 만들기
            # 사용자가 체크한 카테고리 값 (체크박스 상태에 따라 "0" 또는 "1")
            place['category'] = request.form.get(category_key, "0")  # "0"은 기본값, 체크박스가 있으면 "1"로 변경
        
        # 3. 선택된 장소 정보를 세션에 저장
        session['selectedPlaces'] = selected_places  # 선택된 장소 정보 저장

        # 4. 선택된 장소 정보를 results.json 파일에 저장
        with open('results.json', 'w', encoding='utf-8') as f:
            json.dump(selected_places, f, ensure_ascii=False, indent=4)

        # 5. 사용자가 입력한 검색어로 검색을 수행
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
                results = response.json().get('items', [])
                # 검색된 항목에 대해 카테고리 값을 처리
                for item in results:
                    item['category'] = "0"  # 카테고리 기본값 설정 (사용자 체크박스에서 설정할 예정)

                # 세션에 검색된 결과 저장
                session['searchResults'] = results  # 세션에 검색된 결과 저장

                # 검색된 항목을 results.json에 저장
                with open('results.json', 'w', encoding='utf-8') as f:
                    json.dump(results, f, ensure_ascii=False, indent=4)

    else:
        results = []

    # 세션에 저장된 선택된 장소들을 불러옴
    selected_places = session.get('selectedPlaces', [])

    return render_template('index.html', results=results, selected_places=selected_places)
    # index.html 템플릿에 검색 결과를 넘겨줌


@app.route('/save_results', methods=['POST'])
def save_results():
    data = request.json
    selected_places = data.get('selectedPlaces', [])

    # 1. 선택된 장소에 카테고리 값이 잘 반영되어 있는지 확인 (디버깅용)
    print(f"Selected Places received: {selected_places}")

    # 2. 세션에 선택된 장소 정보 저장
    session['selectedPlaces'] = selected_places
    # 3. 선택된 장소 정보를 results.json 파일에 저장
    try:
        with open('results.json', 'w', encoding='utf-8') as f:
            json.dump(selected_places, f, ensure_ascii=False, indent=4)
        print("Selected places saved to results.json")  # 디버깅용 출력
    except Exception as e:
        print(f"Error saving to results.json: {e}")

    return jsonify({"status": "success", "message": "Results saved successfully!"})


# 최적의 경로 페이지로 이동 시 파일들 실행하도록 함. (김현경)
@app.route('/find_route', methods=['POST'])
def find_route():
    try:
        # 현재 디렉토리를 기준으로 파일 경로 설정
        base_dir = os.path.dirname(os.path.abspath(__file__))  # app.py의 절대 경로
        distance_find_path = os.path.join(base_dir, 'distance_find.py')  # distance_find.py 파일 경로
        dijkstra_path = os.path.join(base_dir, 'dijkstra.c')  # dijkstra.c 파일 경로

        # distance_find.py 실행
        subprocess.run(['python', distance_find_path], check=True)  # Python 스크립트 실행

        # dijkstra.c 컴파일 및 실행
        subprocess.run(['gcc', '-o', 'dijkstra', dijkstra_path, 'parson.c'], check=True)  # dijkstra.c 컴파일
        subprocess.run(['./dijkstra'], check=True)  # 컴파일된 실행 파일 실행

        return jsonify({'status': 'success'})

    except subprocess.CalledProcessError as e:
        # 실행 도중 오류 발생 시
        print("Error during execution:", e)
        return jsonify({'status': 'error', 'message': f'실행 중 오류가 발생했습니다: {str(e)}'}), 500

    except Exception as e:
        # 기타 오류 처리
        print("Unexpected error:", e)
        return jsonify({'status': 'error', 'message': f'알 수 없는 오류: {str(e)}'}), 500

@app.route('/optimal_route')
def optimal_route():
    # JSON 파일 읽기
    with open('results.json', 'r', encoding='utf-8') as f:
        results = json.load(f)
    
    with open('최적의_경로.json', 'r', encoding='utf-8') as f:
        optimal_route_data = json.load(f)
    
    # "path"에 있는 인덱스로 results.json에서 여행지 이름을 추출
    path = optimal_route_data.get('path', [])
    
    # results가 리스트가 아니라면, 인덱스를 사용하여 직접 접근
    travel_order = []
    for index in path:
        if 0 <= index < len(results):  # 인덱스 유효성 확인
            travel_order.append(results[index]['name'])
        else:
            print(f"Warning: Index {index} is out of range for `results`.")  # 디버깅 로그

    return render_template('optimal_route.html', route_data=travel_order)

@app.route('/reset')
def reset():
    resetall()
    return redirect(url_for('indexnext'))

# Flask 애플리케이션 실행 (디버그 모드와 포트 설정 포함)
if __name__ == '__main__':
    app.run(debug=True, port=5001)
