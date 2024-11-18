function addToList(name) {
    const list = document.getElementById('travelList');
    const items = list.getElementsByTagName('li');

    // 첫 번째 항목일 때만 "출발지" 표시를 추가
    const isFirstItem = (items.length === 0);
    const listItem = document.createElement('li');
    listItem.textContent = name;

    if (isFirstItem) {
        listItem.classList.add('start-point'); // 첫 번째 항목에 클래스 추가
        listItem.textContent += ' (출발지)'; // "출발지" 텍스트 추가
    }

    const deleteBtn = document.createElement('span');
    deleteBtn.textContent = ' x';
    deleteBtn.className = 'delete-btn';
    deleteBtn.onclick = function () {
        list.removeChild(listItem);
    };

    listItem.appendChild(deleteBtn);
    list.appendChild(listItem);
}
