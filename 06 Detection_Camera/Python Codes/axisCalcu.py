import cv2

def find_white_rectangle(image_path):
    # 载入图像
    image = cv2.imread(image_path)
    if image is None:
        return None

    # 转换为灰度图
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

    # 二值化图像：将灰度图中的白色区域变为白色(255)，其他区域变为黑色(0)
    _, binary = cv2.threshold(gray, 254, 255, cv2.THRESH_BINARY)

    # 查找轮廓
    contours, _ = cv2.findContours(binary, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    # 如果找到轮廓，计算最小矩形边界
    if contours:
        # 假设最大的轮廓就是我们需要的白色区域
        c = max(contours, key=cv2.contourArea)
        x, y, w, h = cv2.boundingRect(c)
        return (x, y), (x+w, y+h)
    else:
        return None

# 使用示例
rectangle_coords = find_white_rectangle("/Users/openCV_test/1204/3.png")
if rectangle_coords:
    print(f"Left top corner: {rectangle_coords[0]}, Right bottom corner: {rectangle_coords[1]}")
else:
    print("No white rectangle found.")
