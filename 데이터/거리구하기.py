from google.colab import drive
drive.mount('/content/drive')


from ast import JoinedStr
import requests
import json

global client_id
global client_secret

client_id = "l10kq6x6md"  # 클라이언트 ID
client_secret = "B42VmUxX7qTtnmwcukOKBm9qKwu158D14VygAIUy"  # 클라이언트 시크릿

def get_distance_and_time(start_lat, start_lon, goal_lat, goal_lon):
    """
    Naver Directions API를 사용하여 두 지점 사이의 거리와 시간을 계산하는 함수
    :param start_lat: 출발지의 위도
    :param start_lon: 출발지의 경도
    :param goal_lat: 도착지의 위도
    :param goal_lon: 도착지의 경도
    :param client_id: Naver Developers에서 발급받은 클라이언트 ID
    :param client_secret: Naver Developers에서 발급받은 클라이언트 시크릿
    :return: 거리(m)
    """
    url = "https://naveropenapi.apigw.ntruss.com/map-direction/v1/driving"
    headers = {
        "X-NCP-APIGW-API-KEY-ID": client_id,
        "X-NCP-APIGW-API-KEY": client_secret
    }
    params = {
        "start": f"{start_lon},{start_lat}",
        "goal": f"{goal_lon},{goal_lat}",
        "option": "fast"  # 가장 빠른 경로 옵션
    }

    response = requests.get(url, headers=headers, params=params)

    if response.status_code == 200:
        data = response.json()
        if "route" in data and "traoptimal" in data["route"]:
            distance = data["route"]["traoptimal"][0]["summary"]["distance"]  # 거리 (미터)
            return distance
        else:
            print("경로를 찾을 수 없습니다.")
            return None
    else:
        print("API 요청 실패:", response.status_code)
        return None

#주소.json파일 받아오기

location_path='/content/drive/MyDrive/플밍2/주소.json'

with open(location_path, 'r', encoding='utf-8') as file:
    data = json.load(file)
#주소 갯수 구하기
n=len(data)
#각 주소별 노드 0~(n-1)에 위도,경도 값 2차원리스트로 받기
locations=[[entry["위도"], entry["경도"]] for entry in data]
#거리 입력할 리스트 주소 개수 * 주소 개수 크기로 생성하기
거리json=[[0 for j in range(n)] for i in range(n)]

for i in range(n):
  for j  in range(n):
    if i==j: # 같은 장소로의 이동은 거리 입력 안함
      continue   
    else:
      start_lat, start_lon = locations[i][0],locations[i][1]   # 출발지 좌표
      goal_lat, goal_lon = locations[j][0],locations[j][1] # 도착지 좌표

      result = get_distance_and_time(start_lat, start_lon, goal_lat, goal_lon)
      거리json[i][j]=result

#거리.json파일 생성
file_path='/content/drive/MyDrive/플밍2/거리.json'
#거리.json파일에 거리json 2차원 배열로 입력
with open(file_path, 'w', encoding='UTF-8') as json_file:
    json.dump(거리json, json_file, ensure_ascii=False, indent=4)