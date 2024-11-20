import json
import requests
from typing import List, Dict, Tuple, Optional

def get_coordinates(address: str, client_id: str, client_secret: str) -> Optional[Tuple[str, str]]:
    """
    Naver Geocoding API를 사용하여 주소를 좌표로 변환하는 함수
    """
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
    """
    Naver Directions API를 사용하여 두 좌표 간 거리를 계산하는 함수
    """
    url = "https://naveropenapi.apigw.ntruss.com/map-direction/v1/driving"
    headers = {
        "X-NCP-APIGW-API-KEY-ID": client_id,
        "X-NCP-APIGW-API-KEY": client_secret
    }
    params = {
        "start": f"{start[1]},{start[0]}",
        "goal": f"{end[1]},{end[0]}",
        "option": "trafast"  # 가장 빠른 경로를 찾는 옵션
    }

    response = requests.get(url, headers=headers, params=params)
    
    if response.status_code == 200:
        data = response.json()
        if data['route']['trafast']:
            distance = data['route']['trafast'][0]['summary']['distance'] / 1000  # meters to kilometers
            return distance
        else:
            print("경로를 찾을 수 없습니다.")
            return None
    else:
        print("API 요청 실패:", response.status_code)
        return None

def calculate_distances_between_all_nodes(coordinates: List[Tuple[str, str]], client_id: str, client_secret: str) -> List[Dict[str, any]]:
    """
    모든 좌표 간 거리를 계산하여 노드 형식으로 저장하는 함수
    """
    distances = []
    num_coordinates = len(coordinates)
    
    for i in range(num_coordinates):
        for j in range(i + 1, num_coordinates):
            start = coordinates[i]
            end = coordinates[j]
            distance = calculate_distance(start, end, client_id, client_secret)
            if distance is not None:
                distances.append({
                    "출발지": f"Node {i + 1}",
                    "도착지": f"Node {j + 1}",
                    "거리 (km)": distance
                })
    
    return distances

def save_distances_to_json(file_path: str, distances: List[Dict[str, any]]):
    """
    거리 데이터를 JSON 파일로 저장하는 함수
    """
    with open(file_path, mode='w', encoding='utf-8') as file:
        json.dump(distances, file, ensure_ascii=False, indent=4)
    print(f"거리가 {file_path}에 저장되었습니다.")

# 사용 예시
client_id = "479rqju7wq"  # 실제 발급받은 클라이언트 ID 입력
client_secret = "Bf0dUPBBzbK55YwEb5f0zKFkjhPgu5Ugag7tHf6m"  # 실제 발급받은 클라이언트 시크릿 입력

# 주소 리스트 입력
address_list_input = input("주소 리스트를 입력하세요 (주소는 쉼표로 구분): ")
address_list = [addr.strip() for addr in address_list_input.split(",")]

# 좌표 리스트 생성
coordinates = []
for address in address_list:
    coord = get_coordinates(address, client_id, client_secret)
    if coord:
        coordinates.append(coord)

# 모든 노드 간 거리 계산
distances = calculate_distances_between_all_nodes(coordinates, client_id, client_secret)

# 거리 데이터를 JSON 파일로 저장
save_distances_to_json("거리.json", distances)
