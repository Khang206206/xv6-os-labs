# Hướng dẫn làm việc nhóm — Lab0

Nhóm gồm **Khang, Phú, Phúc**. Yêu cầu chấm bài theo [đề Lab0](docs/Lab0_System_Call.md). Xem [kế hoạch chi tiết](docs/Ke_hoach_Lab0.md) để biết checklist, giao diện đã chốt, cách tự kiểm tra và yêu cầu nộp bài.

## Source nền

Bài làm dùng nhánh syscall của xv6-labs-2023, commit:

```text
e3b3f8f9b0b1dc9c9e42622b90e2155ff7f7b40a
```

Tag `lab0-base` giữ nguyên tại commit gốc trên để xuất patch. **Điểm bắt đầu code là main đã có sửa tương thích và tài liệu nhóm**, không phải tag lab0-base. Khang tạo cả ba nhánh tính năng từ cùng main đã chuẩn bị xong, trước khi tích hợp code trace.

Source nhóm đã có commit `69f71ac` sửa tương thích:

- `user/usertests.c`: đổi chữ ký thành `rwsbrk(char *s)` để khớp bảng test.
- `Makefile`: thêm `-march=rv64gc -mabi=lp64d` cho cả CFLAGS và ASFLAGS để file C và assembly dùng tập lệnh phù hợp.

Phú và Phúc lấy sẵn các sửa này qua repo nhóm, không sửa lại hoặc bỏ chúng khi làm bài. Khi Phú thêm trace vào Makefile, giữ nguyên cấu hình tương thích.

## Kiểm tra môi trường trước khi code

Không bắt buộc mọi người có GCC/QEMU cùng phiên bản. Môi trường Khang đã kiểm chứng là WSL/Ubuntu, GCC 15.2.0 và QEMU 10.2.1; xv6 boot được và `usertests rwsbrk` báo ALL TESTS PASSED. Đây là kiểm tra source nền, chưa phải kiểm thử chức năng trace hay toàn bộ usertests.

Sau khi clone và chọn nhánh được giao, mở terminal Ubuntu tại thư mục repo, chạy:

```bash
make clean
make qemu
```

Trong shell xv6:

```text
usertests rwsbrk
```

Kỳ vọng test báo OK và ALL TESTS PASSED. Thoát QEMU bằng Ctrl+A, thả phím rồi X. Nếu có lỗi, gửi log cùng kết quả `riscv64-linux-gnu-gcc --version` và `qemu-system-riscv64 --version` cho Khang; không tự hạ/nâng phiên bản chỉ để giống máy Khang.

## Repo và nhánh

Dùng **một repo GitHub Private chung**. Mỗi người dùng tài khoản và danh tính Git của chính mình.

| Nhánh | Người phụ trách | Phạm vi |
|---|---|---|
| `main` | Khang | Tích hợp, chốt bản nộp |
| `feat/trace-user` | Phú | `user/trace.c`, `user/user.h`, `user/usys.pl`, `Makefile` |
| `feat/trace-handler` | Phúc | `kernel/proc.h`, `kernel/proc.c`, `kernel/sysproc.c`, `kernel/syscall.h` |
| `feat/trace-dispatcher` | Khang | `kernel/syscall.c` |
| `fix/trace-integration` | Khang | Chỉ tạo khi cần sửa lỗi sau tích hợp |

Giữ remote MIT dưới tên `upstream`; `origin` là repo GitHub nhóm sau khi thiết lập. Không thay remote trước khi biết đúng URL repo nhóm.

## Giao diện đã chốt

- User: `int trace(int);`, stub `entry("trace");`.
- Kernel: `SYS_trace = 22`, trường `int trace_mask;` trong process, khởi tạo 0.
- Handler: `uint64 sys_trace(void)` đọc mask bằng argint, lưu vào process gọi, trả 0.
- Dispatcher: sau khi handler trả về, kiểm tra bit và in `syscall <tên> -> <giá trị trả về>`.
- Không PID, không kế thừa mask qua fork, không sysinfo. Giữ mask qua exec của process gọi.

Không đổi giao diện đơn phương. Không sửa file của người khác trong phần code riêng hoặc chép thêm phần còn thiếu để ép nhánh riêng chạy được.

## Làm việc và gửi Pull Request

1. Checkout nhánh được giao; sửa đúng phạm vi, kiểm tra phần mình và commit.
2. Push nhánh lên origin, mở Pull Request vào main. Không cần đợi người khác xong mới mở PR.
3. Ghi rõ trong PR: phần đã làm, file thay đổi, lệnh kiểm tra/kết quả, điều chưa kiểm tra và lỗi còn biết.
4. Khang review và merge bằng **Create a merge commit**; không squash để giữ rõ commit từng người.
5. Sau khi ghép đủ ba phần, Khang build, kiểm thử và sửa lỗi cuối trên nhánh fix nếu cần; các sửa lỗi đi qua PR riêng.

Sau thiết lập ban đầu, không push trực tiếp main, không push nhánh của người khác, không force-push lịch sử đã chia sẻ. Nếu cần đổi cùng một file sau bàn giao, trao đổi với Khang trước.

Từng phần có thể chưa build hoặc chưa chạy đầy đủ do thiếu phần còn lại. Ghi đúng giới hạn này; main chỉ sẵn sàng nộp khi ghép đủ và đã kiểm thử. Không cần viết code giả hoặc dựng CI để làm bài.

## Kiểm tra và giữ source sạch

- Phú tự kiểm tra: `make user/trace.o`.
- Phúc tự kiểm tra: `make kernel/proc.o kernel/sysproc.o`.
- Khang rà dispatcher theo giao diện đã chốt; biên dịch dispatcher sau khi có phần handler/process.
- Trước khi commit: xem `git diff`, `git diff --check` và `git status --short`; chỉ add file cần thiết.
- Sau tích hợp: `make clean`, `make qemu`, rồi chạy ba ví dụ trong đề ở shell xv6:

```text
trace 32 grep hello README
trace 2147483647 grep hello README
grep hello README
```

Giữ nguyên README vì đó là dữ liệu đầu vào của ví dụ. Không sửa `user/usys.S` sinh tự động. Không commit file build, ZIP nộp bài hay bản sao thư mục source; .gitignore hiện có đã loại các loại file build chính và ZIP.

## Báo cáo và nộp bài

**Phú và Phúc cùng chịu trách nhiệm hoàn thiện PDF:**

- **Phú:** mục tiêu/phạm vi, thông tin nhóm, phần user; chuẩn bị bố cục và kiểm tra hiển thị PDF.
- **Phúc:** phần kernel, mô tả dispatcher từ ghi chú Khang và bảng kiểm thử từ log thật; ghép nội dung, xuất PDF đúng tên.
- Hai người viết phần riêng độc lập, đọc chéo và cùng duyệt bản cuối trước khi gửi Khang. Nếu cần sửa PDF, hai người xử lý; không chuyển việc biên soạn/định dạng cho Khang.

**Khang một mình phụ trách tạo và kiểm tra patch cuối**, đồng thời giữ vai trò code dispatcher, tích hợp, kiểm thử, sửa lỗi cuối, đóng gói source và nộp Moodle. Khang cung cấp ghi chú kỹ thuật và kết quả test cho hai bạn viết báo cáo. Phân công code và tên nhánh không thay đổi.

GitHub không thay thế việc nộp PDF, diff patch và source sạch theo đề.

Patch cuối được tạo từ `lab0-base` tới main đã kiểm thử, bao gồm cả các thay đổi đã commit. Xem lệnh đóng gói và quy tắc tên MSSV trong kế hoạch. Không dùng số dòng code để thay thế việc ghi nhận đóng góp thực tế.

Kế hoạch trong `docs/Ke_hoach_Lab0.md` là bản chia sẻ qua repo; khi cập nhật quyết định nhóm, cập nhật bản này để mọi người cùng xem được.
