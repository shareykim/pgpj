import json
import requests
from typing import List, Dict, Tuple, Optional

def get_coordinates(address: str, client_id: str, client_secret: str) -> Optional[Tuple[str, str]]:
    """
    Naver Geocoding API를 사용하여 주소를 좌표로 변환하는 함수
    :param address: 주소 (문자열)
    :param client_id: Naver Developers에서 발급받은 클라이언트 ID (문자열)
    :param client_secret: Naver Developers에서 발급받은 클라이언트 시크릿 (문자열)
    :return: (위도, 경도) 형태의 튜플 또는 None
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

def save_addresses_and_coordinates_to_json(file_path: str, address_list: List[str], client_id: str, client_secret: str):
    """
    주소 리스트와 각 주소의 좌표를 JSON 파일에 저장하는 함수
    """
    addresses: List[Dict[str, str]] = []
    
    for address in address_list:
        # 입력된 주소에 대해 좌표를 얻기
        coordinates = get_coordinates(address, client_id, client_secret)
        if coordinates:
            addresses.append({"주소": address, "위도": coordinates[0], "경도": coordinates[1]})

    with open(file_path, mode='w', encoding='utf-8') as file:
        json.dump(addresses, file, ensure_ascii=False, indent=4)
    
    print(f"{len(address_list)}개의 주소와 좌표가 {file_path}에 저장되었습니다.")

# 사용 예시
client_id = "479rqju7wq"  # 실제 발급받은 클라이언트 ID 입력
client_secret = "Bf0dUPBBzbK55YwEb5f0zKFkjhPgu5Ugag7tHf6m"  # 실제 발급받은 클라이언트 시크릿 입력

# 사용자로부터 주소 리스트 입력
address_list_input = input("주소 리스트를 입력하세요 (주소는 쉼표로 구분): ")
address_list = [addr.strip() for addr in address_list_input.split(",")]

# 주소 리스트를 이용해 JSON 파일로 저장
save_addresses_and_coordinates_to_json("addresses_and_coordinates.json", address_list, client_id, client_secret)
