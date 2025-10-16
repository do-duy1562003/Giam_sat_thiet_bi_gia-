# Giam_sat_thiet_bi_gia_dinh
 Giới Thiệu
Hệ thống này cung cấp một giải pháp tích hợp để:

Điều khiển các thiết bị (đèn, quạt, điều hòa, v.v.) từ xa
Giám sát cảm biến (nhiệt độ, độ ẩm, chuyển động, v.v.) thời gian thực
Nhận cảnh báo tức thì khi có sự cố hoặc vượt ngưỡng an toàn
Lên lịch các công việc tự động
Ghi nhật ký tất cả hoạt động để phân tích và kiểm tra


✨ Tính Năng Chính
1. 🎛️ Điều Khiển Thiết Bị Thông Minh
Loại Thiết Bị Được Hỗ Trợ:

Đèn thông minh: Bật/tắt, điều chỉnh độ sáng, thay đổi màu sắc
Điều hòa không khí: Điều chỉnh nhiệt độ, chế độ làm lạnh/sưởi
Quạt: Điều chỉnh tốc độ (bật, tắt, chế độ đêm)
Ổ cắm điện: Bật/tắt nguồn cho các thiết bị
Khóa cửa: Mở/khóa từ xa với xác thực
Bộ lọc không khí: Bật/tắt, kiểm tra mức bụi
Máy bơm nước: Điều khiển tưới cây tự động

Cách Điều Khiển:

Ứng dụng Web Dashboard: Giao diện trực quan, thao tác đơn giản
Ứng dụng Mobile: Điều khiển từ smartphone iOS/Android
Giọng nói: Tích hợp Amazon Alexa, Google Home
MQTT Protocol: Cho các ứng dụng tùy chỉnh
REST API: Tích hợp với hệ thống bên thứ ba

2. 📊 Giám Sát Cảm Biến
Các Cảm Biến Được Hỗ Trợ:

DHT22/AM2302: Nhiệt độ và độ ẩm
BMP280: Áp suất khí quyển
Light Sensor (LDR): Cường độ ánh sáng
PIR Motion Sensor: Phát hiện chuyển động
MQ-2/MQ-5: Phát hiện khí gas/khí tự nhiên
Water Level Sensor: Cảm biến mức nước
Door/Window Sensor: Cảm biến cửa/cửa sổ
Soil Moisture: Độ ẩm đất cho cây

Dữ Liệu Giám Sát:

Hiển thị giá trị cảm biến theo thời gian thực
Biểu đồ lịch sử dữ liệu với các khoảng thời gian khác nhau
Thống kê min/max/average
Cảnh báo khi vượt ngưỡng an toàn

3. 🚨 Hệ Thống Cảnh Báo Thông Minh
Loại Cảnh Báo:
Sự KiệnMức ĐộHành ĐộngNhiệt độ cao > 40°C⚠️ CaoSMS, Push Notification, EmailNhiệt độ thấp < 10°C⚠️ CaoSMS, Push Notification, EmailPhát hiện chuyển động (đêm)🔴 Cảnh báoBật đèn, ghi hình, gọi điệnCửa/Cửa sổ mở khi vắng nhà🔴 Cảnh báoSMS tức thì, bật báo độngPhát hiện khí gas/khí tự nhiên🔴 Cảnh báoSMS tức thì, mở cửa sổ, cắt gasThiết bị ngừng hoạt động⚠️ Trung bìnhEmail, Push NotificationMất kết nối mạng⚠️ Trung bìnhLưu vào log, cố gắng kết nối lạiCamera offline⚠️ Trung bìnhEmail, Push Notification
Kênh Thông Báo:

SMS: Cho các cảnh báo nghiêm trọng (tích hợp Twilio)
Push Notification: Qua ứng dụng mobile
Email: Báo cáo chi tiết
Telegram/Discord: Cảnh báo tức thì qua bot
Gọi điện: Cho các tình huống khẩn cấp

4. ⏰ Lên Lịch Tự Động
Các Loại Lịch:

Lịch Hàng Ngày: Bật/tắt đèn vào giờ cụ thể
Lịch Hàng Tuần: Thiết lập lịch cho từng ngày trong tuần
Lịch Hàng Tháng: Thực hiện các công việc định kỳ
Cảnh Sáng: Tạo các cảnh tự động (vd: "Cảnh buổi sáng")
Điều Kiện Có Điều Kiện: IF-THEN (nếu nhiệt độ > 30°C thì bật AC)

Ví Dụ:
- 6:00 AM: Bật đèn phòng ngủ 50%
- 7:00 AM: Bật quạt + bộ lọc không khí
- 8:00 AM: Tắt tất cả đèn (ra khỏi nhà)
- 17:00 PM: Bật đèn phòng khách + đèn bếp
- 22:00 PM: Tắt tất cả, khóa cửa
5. 📱 Giao Diện Người Dùng
Dashboard Web:

Bảng điều khiển trực quan với các widget
Biểu đồ thời gian thực
Danh sách lịch sử hoạt động
Quản lý người dùng và quyền hạn

Ứng Dụng Mobile:

Điều khiển nhanh (Quick Control)
Thông báo push tức thời
Chế độ tiết kiệm pin
Hoạt động offline (điều khiển cơ bản)
