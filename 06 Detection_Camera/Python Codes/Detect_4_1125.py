from flask import Flask, request, jsonify
import cv2
import numpy as np
import base64


def load_image(image_path):
    """
    load image
    """
    image = cv2.imread(image_path, cv2.IMREAD_COLOR)
    if image is None:
        raise ValueError(f"Unable to load image at {image_path}")
    return image


def decode_image(base64_string):
    """
    decode base64 image to OpenCV image array
    """
    # decode base64
    img_data = base64.b64decode(base64_string)
    # convert to numpy array
    nparr = np.frombuffer(img_data, np.uint8)
    # numpy -> OpenCV
    img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
    return img


def preprocess_image(image):
    """
    preprocess image (BGR to Gray, Blurring)
    """
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    blur = cv2.GaussianBlur(gray, (5, 5), 0)
    return blur


def detect_changes(image1, image2, threshold=30):
    """
    detect the changes between two images

    Args:
        image1: OpenCV image array, the first image from ESP32Camera.
        image2: OpenCV image array, the second image from ESP32Camera.
        threshold: Int from 0 to 255, the threshold that defines the change between the two images.

    Returns:
        OpenCV contours, describe the location of changes between the two images.
    """
    # preprocess images
    processed_image1 = preprocess_image(image1)
    processed_image2 = preprocess_image(image2)

    # get diff
    diff = cv2.absdiff(processed_image1, processed_image2)
    _, thresh = cv2.threshold(diff, threshold, 255, cv2.THRESH_BINARY)

    # find contours
    contours, _ = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)

    # filter contours
    new_contours = []
    for contour in contours:
        if cv2.contourArea(contour) < 4000:
            continue
        print("Detected Changed Areas:",cv2.contourArea(contour))
        new_contours.append(contour)
    return new_contours


def detect_level_of_change(contour, level_bounds):
    """
    detect the changes between two images

    Args:
        contour: OpenCV contours, describe the location of changes between the two images.
        level_bounds: the bounding box of pre-defined areas, [(x1,y1), (x2,y2)].

    Returns:
        Bool, whether or not the changes are in the given bounds.
    """
    (x, y, w, h) = cv2.boundingRect(contour)
    contour_bounds = (x, y), (x + w, y + h)

    # check if the changes are in the given bounds
    if (contour_bounds[0][0] >= level_bounds[0][0] and contour_bounds[1][0] <= level_bounds[1][0] and
            contour_bounds[0][1] >= level_bounds[0][1] and contour_bounds[1][1] <= level_bounds[1][1]):
        return True
    return False


app = Flask(__name__)

@app.route('/compare-images', methods=['POST'])
def compare_images():
    data = request.json
    print("received", data)
    if 'photo1' not in data or 'photo2' not in data:
        return 'Missing images', 400

    img1 = decode_image(data['photo1'])
    img2 = decode_image(data['photo2'])

    # Process and compare images here
    detect_contours = detect_changes(img1, img2)
    if len(detect_contours)==0:
        print('Do not detect any differences between the two images')

    first_level_bounds = [(71, 27), (450, 225)]  # level 1: left top & bottom right
    second_level_bounds = [(160, 180), (450, 327)]  # level 2: left top & bottom right
    third_level_bounds = [(214, 286), (439, 404)]  # level 3: left top & bottom right

    # detect_changes
    change_levels = []
    for contour in detect_contours:
        if detect_level_of_change(contour, first_level_bounds):
            change_levels.append(1)
            print("Change detected in First Level")
        elif detect_level_of_change(contour, second_level_bounds):
            change_levels.append(2)
            print("Change detected in Second Level")
        elif detect_level_of_change(contour, third_level_bounds):
            change_levels.append(3)
            print("Change detected in Third Level")

    print("changes are detected in these levels:", change_levels)
    return jsonify({'isDetected': 1, 'detectedLevel': change_levels})


if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=9090)

    # TestTest
    # image1 = load_image("/Users/openCV_test/test2/20231119220902.jpg") # path of photo 1
    # image2 = load_image("/Users/openCV_test/test2/20231119220910.jpg") # path of photo 2

