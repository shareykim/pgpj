import json
import requests
import time
from typing import List, Tuple, Optional

def get_coordinates(address: str, client_id: str, client_secret: str) -> Optional[Tuple[str, str]]:
    url = "https://naveropenapi.apigw.ntruss.com/map-geocode/v2/geocode"
    headers = {
        "X-NCP-APIGW-API-KEY-ID": client_id,
        "X-NCP-APIGW-API-KEY": client_secret
    }
    params = {"query": address}

    response = requests.get(url, headers=headers, params=params)

    if response.status_code == 200:
        data = response.json()
        if data['addresses']:
            lat = data['addresses'][0]['y']
            lon = data['addresses'][0]['x']
            return lat, lon
        else:
            print(f"'{address}' 주소를 찾을 수 없습니다.")
            return None
    else:
        print("API 요청 실패:", response.status_code)
        return None

def calculate_distance(start: Tuple[str, str], end: Tuple[str, str], client_id: str, client_secret: str) -> Optional[float]:
    url = "https://naveropenapi.apigw.ntruss.com/map-direction/v1/driving"
    headers = {
        "X-NCP-APIGW-API-KEY-ID": client_id,
        "X-NCP-APIGW-API-KEY": client_secret
    }
    params = {
        "start": f"{start[1]},{start[0]}",
        "goal": f"{end[1]},{end[0]}",
        "option": "trafast"
    }

    response = requests.get(url, headers=headers, params=params)
    
    if response.status_code == 200:
        data = response.json()
        if 'route' in data and 'trafast' in data['route'] and data['route']['trafast']:
            distance = data['route']['trafast'][0]['summary']['distance'] / 1000
            return distance
        else:
            print(f"'{start}'에서 '{end}'로의 경로를 찾을 수 없습니다.")
            return None
    else:
        print("API 요청 실패:", response.status_code)
        return None

def calculate_distances_matrix(coordinates: List[Tuple[str, str]], client_id: str, client_secret: str) -> List[List[float]]:
    num_coordinates = len(coordinates)
    matrix = [[0 for _ in range(num_coordinates)] for _ in range(num_coordinates)]

    for i in range(num_coordinates):
        for j in range(num_coordinates):
            if i == j:
                matrix[i][j] = 0
            elif matrix[i][j] == 0 and matrix[j][i] == 0:
                start = coordinates[i]
                end = coordinates[j]
                distance = calculate_distance(start, end, client_id, client_secret)
                if distance is not None:
                    matrix[i][j] = distance
                    matrix[j][i] = distance
                time.sleep(0.1)  # 호출 간 간격 추가

    return matrix

def save_distances_to_json(file_path: str, distances_matrix: List[List[float]]):
    data = {
        "weight": distances_matrix
    }
    with open(file_path, mode='w', encoding='utf-8') as file:
        json.dump(data, file, ensure_ascii=False, indent=4)
    print(f"거리가 {file_path}에 저장되었습니다.")

# 사용 예시
client_id = 'l10kq6x6md'
client_secret = 'B42VmUxX7qTtnmwcukOKBm9qKwu158D14VygAIUy'

# `results`에서 주소 리스트 추출
with open('results.json', 'r', encoding='utf-8') as f:
    results = json.load(f)
address_list = [item['address'] for item in results if 'address' in item]

# 좌표 리스트 생성
coordinates = []
for address in address_list:
    coord = get_coordinates(address, client_id, client_secret)
    if coord:
        coordinates.append(coord)

# 모든 노드 간 거리 계산
distances = calculate_distances_matrix(coordinates, client_id, client_secret)

# 거리 데이터를 JSON 파일로 저장
save_distances_to_json("거리.json", distances)