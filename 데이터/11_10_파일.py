import json
import requests
from typing import List, Tuple, Optional, Union

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
        "option": "trafast"
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

def calculate_distance_matrix(coordinates: List[Tuple[str, str]], client_id: str, client_secret: str) -> List[List[Union[str, float]]]:
    """
    좌표 리스트를 기반으로 거리 행렬을 계산하여 반환하는 함수
    """
    num_nodes = len(coordinates)
    distance_matrix = [["INF" for _ in range(num_nodes)] for _ in range(num_nodes)]

    for i in range(num_nodes):
        for j in range(num_nodes):
            if i == j:
                distance_matrix[i][j] = 0  # 자기 자신까지의 거리는 0
            elif distance_matrix[i][j] == "INF":
                distance = calculate_distance(coordinates[i], coordinates[j], client_id, client_secret)
                if distance is not None:
                    distance_matrix[i][j] = distance
                    distance_matrix[j][i] = distance  # 대칭 행렬이므로

    return distance_matrix

def save_distance_matrix_to_json(file_path: str, distance_matrix: List[List[Union[str, float]]]):
    """
    거리 행렬을 JSON 파일로 저장하는 함수
    """
    data = {"weight": distance_matrix}
    with open(file_path, mode='w', encoding='utf-8') as file:
        json.dump(data, file, ensure_ascii=False, indent=4)
    print(f"거리 행렬이 {file_path}에 저장되었습니다.")

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

# 거리 행렬 계산
distance_matrix = calculate_distance_matrix(coordinates, client_id, client_secret)

# 거리 행렬을 JSON 파일로 저장
save_distance_matrix_to_json("distance_matrix.json", distance_matrix)
