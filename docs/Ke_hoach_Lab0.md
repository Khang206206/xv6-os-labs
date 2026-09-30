# Kế hoạch Lab0 — Khang, Phú, Phúc làm độc lập và tích hợp cuối

Cập nhật: 30/09/2026. Yêu cầu bài tập căn cứ vào `D:\Lab0_docs\Lab0_System_Call.md`. Phân công và giao diện dưới đây đã được Khang đồng ý.

## 1. Phạm vi và cách làm

Chỉ thực hiện **trace — 10 điểm** theo đề: system call nhận mask, lưu theo process gọi, in tên syscall và giá trị trả về; chương trình user gọi trace rồi exec chương trình đích. Output dạng `syscall read -> 1023`, không PID. Không thêm kế thừa mask qua fork, sysinfo, bài hello riêng, câu hỏi GDB, answers/time của MIT, in tham số syscall hoặc CI. Không lấy điểm grader MIT làm điều kiện hoàn thành.

Cả ba có **phần code riêng trong bản diff cuối**. Nhóm dùng một repository GitHub Private chung. Phú và Phúc clone repo về máy và viết trên nhánh riêng, không chờ code hay lịch làm việc của nhau. Khang cũng có thể viết dispatcher ngay theo giao diện đã chốt, sau đó là người tích hợp, kiểm thử, sửa lỗi cuối và nộp bài.

**Độc lập khi viết không có nghĩa từng phần chạy đầy đủ riêng lẻ.** Kiểm thử toàn luồng cần ghép đủ ba phần. Không viết handler giả, không chép file của bạn khác vào phần bàn giao để ép bản riêng chạy được.

Không ép số dòng code bằng nhau: ba phần đều đóng góp trực tiếp vào chức năng. Phần user đã có code khung nên phải tận dụng, không viết lại cho nhiều dòng. Khang làm thêm tích hợp và một mình phụ trách tạo/kiểm tra patch cuối; Phú và Phúc cùng chịu trách nhiệm viết, kiểm tra và hoàn thiện PDF để chia đều việc báo cáo. Phân công code và nhánh giữ nguyên.

## 2. Giao diện cố định — không cần đợi nhau quyết định

### Source gốc

Nhánh `syscall`, commit đã chốt:

```text
e3b3f8f9b0b1dc9c9e42622b90e2155ff7f7b40a
```

Khang tạo `main` từ commit này và đặt tag `lab0-base` tại đúng commit source gốc; nếu tag đã có thì kiểm tra, không ghi đè. Có thể thêm tài liệu phối hợp bằng một commit riêng trên main trước khi tạo ba nhánh tính năng. Ba nhánh tính năng phải cùng xuất phát từ main ban đầu đó, trước khi ghép bất kỳ phần code trace nào. Không dùng nhánh util hoặc tự chuyển sang phiên bản xv6 khác.

### Giao diện giữa các phần

| Thành phần | Quy ước cố định | Người cài đặt |
|---|---|---|
| Hàm user | `int trace(int);` | Phú |
| Stub | `entry("trace");` trong `user/usys.pl` | Phú |
| Số syscall | `#define SYS_trace 22` trong `kernel/syscall.h` | Phúc |
| Trường process | `int trace_mask;` trong `struct proc` | Phúc |
| Giá trị khởi tạo | 0 khi cấp process, kể cả tái sử dụng slot | Phúc |
| Handler | `uint64 sys_trace(void)`; dùng `argint(0, &mask)`, lưu mask vào process hiện tại và trả 0 | Phúc |
| Khai báo/đăng ký handler | `extern uint64 sys_trace(void);`, entry `[SYS_trace]` trong bảng handler | Khang |
| In trace | Đọc `p->trace_mask`, kiểm tra bit số syscall, in tên và return value thực tế sau khi handler trả về | Khang |
| Format | `syscall <tên> -> <giá trị trả về>` kèm xuống dòng, không PID | Khang |
| Phạm vi mask | Giữ qua exec của process gọi; không sao chép sang process con qua fork | Phúc bảo đảm trạng thái; Khang kiểm tra |

Số 22 đã được đối chiếu với source gốc: syscall hiện có kết thúc ở `SYS_close = 21`. Không đổi tên, số hay kiểu trong bảng này đơn phương. Nếu thực sự phải đổi, thông báo cả nhóm trước để tránh lệch giao diện.

## 3. Quyền sửa file và sản phẩm từng người

Các đường dẫn dưới đây tính từ thư mục source xv6.

| Người | File được sửa trong phần code riêng | Sản phẩm |
|---|---|---|
| **Phú** | `user/trace.c`, `user/user.h`, `user/usys.pl`, `Makefile` | User program + giao diện user + cấu hình build |
| **Phúc** | `kernel/proc.h`, `kernel/proc.c`, `kernel/sysproc.c`, `kernel/syscall.h` | Trạng thái mask + handler + số syscall |
| **Khang** | `kernel/syscall.c` | Đăng ký handler + bảng tên + logic chọn/in trace |

Phú/Phúc không sửa `kernel/syscall.c`. Khang có thể sửa các file khác khi tích hợp cuối nếu cần sửa lỗi, ghi riêng commit sửa lỗi để giữ rõ đóng góp ban đầu. Không sửa trực tiếp `user/usys.S` vì make sinh lại file này.

Ước lượng phần đọc/code/tự kiểm tra: Phú 3–5 giờ, Phúc 3–5 giờ, Khang 3–5 giờ. Ngoài ra Phú và Phúc mỗi người dành khoảng 1–2 giờ làm phần PDF được giao và kiểm tra chéo; Khang 2–4 giờ tích hợp/test/tạo patch/đóng gói. Đây là dự kiến điều phối, không phải yêu cầu đề hay cam kết số giờ bằng nhau.

## 4. Chuẩn bị — mỗi người tự hoàn thành theo lịch của mình

- [ ] Khởi tạo Ubuntu/WSL, có compiler RISC-V, make và QEMU. Chạy lệnh build/git trong Ubuntu; chạy trace/grep trong shell xv6.
- [ ] Đọc Chương 2 và mục 4.3–4.4 Chương 4 sách xv6 như đề yêu cầu.
- [ ] Kiểm tra working tree trước khi chuyển nhánh; giữ lại công việc đang có, không xóa/ghi đè.
- [ ] Khang chuẩn bị repo GitHub và các nhánh theo mục 8 từ source syscall đã chốt. Phú/Phúc clone repo nhóm sau khi được cấp quyền; không cần tự khởi tạo repo khác.
- [ ] Chọn đúng nhánh: Phú `feat/trace-user`, Phúc `feat/trace-handler`, Khang `feat/trace-dispatcher`. Các nhánh đã được tạo từ cùng main ban đầu, nên mỗi người có thể bắt đầu vào thời điểm khác nhau.
- [ ] Trước khi sửa, chạy `make clean`, `make qemu` trên source gốc và thử `ls`. Thoát bằng `Ctrl-a` rồi `x`.

Sau khi repo và các nhánh ban đầu đã sẵn sàng, ai chuẩn bị xong có thể code ngay. GitHub là cách phối hợp nhóm đã chọn, không phải yêu cầu nộp bài của đề; kết quả cuối vẫn nộp Moodle.

## 5. Phú — chương trình trace và giao diện user

### P1. Đọc và xác định phần cần sửa

- [ ] Đọc `user/trace.c` có sẵn, `user/user.h`, `user/usys.pl` và phần UPROGS trong Makefile.
- [ ] Hiểu luồng `trace(mask)` → stub → ecall, rồi `exec` chương trình đích.
- [ ] Dùng đúng tên trace và số syscall qua stub; không tự thêm số syscall vào header do Phúc phụ trách.

### P2. Viết phần code riêng

- [ ] Thêm prototype `int trace(int);` trong `user/user.h`.
- [ ] Thêm `entry("trace");` trong `user/usys.pl`.
- [ ] Thêm `$U/_trace` vào UPROGS của Makefile, theo cú pháp hiện có.
- [ ] Hoàn thiện `user/trace.c`: lấy mask từ `argv[1]`, gọi trace trước exec, dùng `argv[2]` làm chương trình đích.
- [ ] Truyền các đối số còn lại đúng thứ tự; mảng argv truyền vào exec phải có null kết thúc và không vượt giới hạn.
- [ ] Giữ xử lý lỗi cơ bản: thiếu đối số thì báo usage; trace/exec thất bại thì báo lỗi và thoát thất bại.
- [ ] Tận dụng chương trình khung, không thêm tính năng ngoài đề.

### P3. Tự kiểm tra và bàn giao, không chờ Phúc

- [ ] Chạy `make user/trace.o` trong Ubuntu để kiểm tra biên dịch file C phía user.
- [ ] Rà lại entry stub, UPROGS, thứ tự đối số và đường xử lý exec thất bại.
- [ ] Chạy `git diff --check`; kiểm tra diff chỉ gồm 4 file được giao, không có file sinh tự động.
- [ ] Commit bằng danh tính của Phú, push lên `feat/trace-user` và mở Pull Request vào `main` cho Khang, kèm mô tả và kết quả kiểm tra.

**Giới hạn khi chạy riêng:** chưa có `SYS_trace` từ Phúc nên build toàn bộ stub có thể lỗi; chưa có handler/dispatcher nên chưa chạy được trace đầy đủ. Ghi rõ chưa kiểm tra toàn luồng, không cần chờ hai phần đó để bàn giao code.

### P4. Cùng Phúc làm PDF — nội dung user và trình bày

- [ ] Viết phần mục tiêu/phạm vi, thông tin thành viên/phân công và giải thích user/argv/stub/exec; ghi khó khăn thực tế của phần mình.
- [ ] Chuẩn bị bố cục và định dạng báo cáo ngắn; không chép toàn bộ source.
- [ ] Gửi phần nội dung cho Phúc ghép với phần kernel và kiểm thử. Hai người có thể viết phần riêng theo lịch của mình; không cần đợi nhau mới bắt đầu.
- [ ] Đọc chéo phần kernel và kiểm thử do Phúc viết, kiểm tra tên/MSSV và tính dễ hiểu của toàn báo cáo.
- [ ] Sau khi Phúc xuất bản PDF, kiểm tra hiển thị, bố cục và tên file; cùng Phúc chốt PDF gửi Khang. Việc PDF cuối cần kết quả test thật không chặn bàn giao code.

**Hoàn tất code Phú:** giao diện đúng mục 2, object trace biên dịch được, diff đúng phạm vi và đã gửi Khang. Nghiệm thu chức năng toàn phần do Khang thực hiện sau tích hợp.

## 6. Phúc — mask của process và handler

### C1. Đọc phần cần thiết

- [ ] Đọc `struct proc`, `allocproc()` và `freeproc()` để hiểu slot process được cấp và tái sử dụng thế nào.
- [ ] Đọc handler có sẵn trong `kernel/sysproc.c` và khai báo `argint()`; source này dùng `argint()` trả `void`.
- [ ] Đọc `kernel/syscall.h`; dùng số 22 đã chốt, không cần đợi Phú chọn số.

### C2. Viết phần code riêng

- [ ] Thêm `#define SYS_trace 22` trong `kernel/syscall.h`.
- [ ] Thêm `int trace_mask;` vào `struct proc` trong `kernel/proc.h`.
- [ ] Khởi tạo mask bằng 0 trên đường cấp process, trước khi process có thể chạy; bảo đảm tái sử dụng slot không mang mask cũ.
- [ ] Không sao chép mask cha sang con trong fork. Không dùng mask toàn cục.
- [ ] Viết `uint64 sys_trace(void)` trong `kernel/sysproc.c`: đọc tham số int, gán `myproc()->trace_mask`, trả 0.
- [ ] Kiểm tra exec không xóa trường mask của process đang gọi; chỉ sửa thêm nếu thực sự cần cho yêu cầu này.
- [ ] Không đăng ký handler và không viết logic log trong `kernel/syscall.c`: Khang phụ trách.

### C3. Tự kiểm tra và bàn giao, không chờ Phú

- [ ] Chạy `make kernel/proc.o kernel/sysproc.o` để kiểm tra các file C mình sửa.
- [ ] Rà đường cấp process thành công, tái sử dụng slot và việc giữ mask qua exec bằng source.
- [ ] Kiểm tra số syscall không trùng và handler dùng đúng kiểu/giao diện.
- [ ] Chạy `git diff --check`; kiểm tra diff chỉ gồm 4 file được giao.
- [ ] Commit bằng danh tính của Phúc, push lên `feat/trace-handler` và mở Pull Request vào `main` cho Khang, kèm trạng thái kiểm tra và giải thích ngắn vòng đời mask.

**Giới hạn khi chạy riêng:** handler chưa được dispatcher đăng ký và user chưa có giao diện mới, nên việc object biên dịch được không chứng minh trace chạy đúng. Không viết phần của Phú/Khang để thử gọi handler và không đợi Phú bàn giao.

### C4. Cùng Phú làm PDF — nội dung kernel, kiểm thử và xuất file

- [ ] Viết phần mask/argint/vòng đời process và khó khăn thực tế; có thể làm ngay sau phần code, không đợi Phú.
- [ ] Dựa trên ghi chú dispatcher do Khang cung cấp để viết phần luồng xử lý và in trace; nhờ Khang xác nhận điểm kỹ thuật chưa rõ.
- [ ] Nhận log/kết quả test và hạn chế từ Khang, trình bày bảng kết quả trung thực. Khang cung cấp dữ liệu kỹ thuật, không phải viết hay định dạng PDF.
- [ ] Đọc chéo phần user và thông tin nhóm của Phú; ghép nội dung hai người, kiểm tra không trùng lặp hoặc mâu thuẫn.
- [ ] Xuất `StudentID1_StudentID2_StudentID3_Report.pdf` với MSSV tăng dần; chuyển Phú kiểm tra hiển thị và cùng chốt bản cuối gửi Khang.
- [ ] Nếu có lỗi nội dung/định dạng PDF, Phú và Phúc tự sửa theo phần phụ trách; nếu có lỗi code khó, hỗ trợ Khang khi được hỏi và tránh sửa trùng file.

**Trách nhiệm PDF chung:** Phú phụ trách nội dung user/bố cục/kiểm tra hiển thị; Phúc phụ trách nội dung kernel/bảng test/ghép và xuất PDF. Cả hai duyệt bản cuối. Không sửa đồng thời cùng một bản báo cáo mà chưa thống nhất người đang ghép.

**Hoàn tất code Phúc:** các object biên dịch được, mask/handler đúng giao diện và đã gửi Khang. Không có yêu cầu test kế thừa fork.

## 7. Khang — dispatcher, tích hợp, kiểm thử và sửa lỗi cuối

### K1. Viết phần code riêng ngay theo giao diện đã chốt

- [ ] Đọc `kernel/syscall.c`, danh sách syscall gốc và format hỗ trợ trong `kernel/printf.c`.
- [ ] Khai báo `extern uint64 sys_trace(void);` và thêm ánh xạ `[SYS_trace]` vào bảng handler.
- [ ] Tạo bảng tên syscall khớp các số hiện có, gồm trace ở số 22; ưu tiên chỉ số theo hằng SYS để tránh lệch tên.
- [ ] Trong nhánh syscall hợp lệ, giữ lời gọi handler và ghi kết quả vào `p->trapframe->a0`.
- [ ] Sau đó đọc mask của process hiện tại, kiểm tra bit syscall; nếu bật thì in tên và return value đúng mẫu, kèm xuống dòng.
- [ ] Chỉ tra bảng/dịch bit khi số syscall hợp lệ; xử lý kiểu dữ liệu và format phù hợp để lỗi -1 không bị in sai thành số dương lớn.
- [ ] Kiểm tra trace có thể in chính lời gọi trace khi mask vừa được bật; không lấy mask cũ trước khi handler chạy.
- [ ] Không kỳ vọng dòng log sau return cho syscall không quay về dispatcher như exit.
- [ ] Rà diff, commit riêng phần `kernel/syscall.c` bằng danh tính Khang, push lên `feat/trace-dispatcher` và mở Pull Request vào `main`.

**Tự kiểm tra trước khi ghép:** rà source và `git diff --check`. Build dispatcher cần trường `trace_mask`, số `SYS_trace` và handler từ phần Phúc; không khai báo tạm hoặc viết lại phần Phúc để làm bản riêng build được. Khang có thể hoàn thành phần viết code và chuẩn bị bảng test trước khi nhận bàn giao.

### K2. Tích hợp khi nhận code, không cần hai bạn cùng online

- [ ] Nhận và kiểm tra phạm vi diff của từng người; kiểm tra bắt đầu từ đúng baseline và tuân thủ mục 2.
- [ ] Khang kiểm tra và merge từng Pull Request vào `main` bằng **Create a merge commit**, giữ lịch sử commit của từng người; không dùng squash hoặc gửi patch nội bộ làm luồng chính.
- [ ] Có thể nhận Phú hoặc Phúc trước, không cần chờ để rà diff. Chỉ kiểm thử toàn luồng khi đã ghép đủ ba phần.
- [ ] Chạy `make clean`, `make qemu`; phân biệt lỗi biên dịch, liên kết và lỗi hành vi.
- [ ] Nếu phát hiện lỗi cuối, Khang tạo `fix/trace-integration` từ main đã ghép đủ, sửa file liên quan và commit riêng ghi rõ nội dung sửa; mở PR vào main, kiểm thử rồi merge. Chỉ tạo nhánh fix khi có lỗi. Nếu cần giải thích hoặc sửa lớn, trao đổi đúng người phụ trách, không bắt cả nhóm làm đồng thời.

### K3. Kiểm thử bản đầy đủ

Chạy trong shell xv6:

| Mã | Lệnh | Điều kiện đạt |
|---|---|---|
| T1 | `trace 32 grep hello README` | Chỉ log read; return value thực tế đúng; có read trả 0 khi hết file |
| T2 | `trace 2147483647 grep hello README` | Log trace, exec, open, read, close theo đường chạy thực; đúng tên, kết quả và format không PID |
| T3 | `grep hello README` sau lệnh có trace | Không log syscall; process khác không bị bật trace theo |

- [ ] Chạy đủ ba ví dụ đề; ghi kết quả thực tế, không ghi pass trước khi chạy.
- [ ] Lặp chuỗi có trace rồi không trace để phát hiện mask cũ bị giữ ở slot process.
- [ ] Kiểm tra ngắn `trace 32 echo one two`, thiếu đối số và chương trình không tồn tại để xác nhận argv/xử lý lỗi không hỏng. Đây là kiểm tra code đã viết, không thêm tính năng.
- [ ] Không hard-code số byte read; nếu khác ví dụ, đối chiếu README và kết quả syscall thực. Không sửa README để ép mẫu.
- [ ] Khi sửa lỗi, chạy lại ca bị ảnh hưởng; trước khi chốt bản cuối phải có T1–T3 đạt trên bản cuối.
- [ ] Gửi Phú và Phúc ghi chú dispatcher, log/kết quả test và hạn chế thật để hai bạn viết PDF; Khang không phụ trách biên soạn hoặc định dạng báo cáo.

Không cần chạy grader MIT để đạt điểm, không thêm test kế thừa fork hay framework mới. Nếu phát hiện lỗi có phạm vi rộng hơn thì kiểm tra thêm đúng phần bị ảnh hưởng.

### K4. Đóng gói và nộp

- [ ] Nhận PDF đã được cả Phú và Phúc kiểm tra; đối chiếu nhanh kết quả với code cuối và đủ thành phần nộp. Nếu PDF cần sửa, trả hai bạn chỉnh; Khang không nhận phần biên soạn PDF.
- [ ] Khang là người duy nhất tạo và kiểm tra diff patch nộp bài; Phú/Phúc bàn giao code qua PR, không cần tạo patch nộp riêng.
- [ ] Bảo đảm mọi PR cần nộp đã merge vào `main`; checkout main, cập nhật từ GitHub, kiểm tra working tree sạch và đúng commit cuối đã test trước khi xuất HEAD.
- [ ] Dùng baseline đã chốt để xuất diff gồm cả thay đổi đã commit. Chạy trong Ubuntu, thay MSSV đúng trước khi dùng:

```bash
git status --short
git diff --check lab0-base HEAD
git diff --binary lab0-base HEAD > ../StudentID1_StudentID2_StudentID3.patch
make clean
git archive --format=zip --prefix=xv6-labs-2023/ -o ../xv6-labs-2023.zip HEAD
```

Nếu còn thay đổi cần nộp, commit trước khi xuất bản cuối. `git archive` chỉ chứa file đã commit. `git diff` không chỉ định baseline sau khi đã commit có thể tạo patch rỗng.

- [ ] Kiểm tra patch áp được lên một bản baseline riêng và chứa đủ ba phần code.
- [ ] Giải nén source ZIP ở thư mục riêng, build và chạy T1–T3 để phát hiện thiếu file. Gói source nộp vẫn là ZIP sạch đã xuất, không phải thư mục test vừa build.
- [ ] Phú và Phúc xác nhận PDF đã hoàn tất; Khang kiểm tra patch, source ZIP và gói nộp cuối, không cần tổ chức buổi đồng thời.
- [ ] Đặt tên theo MSSV tăng dần; đóng ZIP tổng gồm các thành phần đề yêu cầu:

```text
StudentID1_StudentID2_StudentID3.zip
├── StudentID1_StudentID2_StudentID3_Report.pdf
├── Release/
│   └── StudentID1_StudentID2_StudentID3.patch
└── Source/
    └── xv6-labs-2023.zip
```

- [ ] Khang nộp Moodle và xác nhận hệ thống nhận bài, báo lại nhóm. Không nộp qua email.

## 8. GitHub — repo, nhánh và quy trình Pull Request

### 8.1. Khang chuẩn bị repo một lần

- [ ] Tạo **một repo GitHub Private**, mời Phú và Phúc làm collaborator để push nhánh của mình. Tên repo và tài khoản GitHub điền khi tạo; kế hoạch chưa giả định URL.
- [ ] Tạo repo GitHub rỗng, không sinh README/license/gitignore mới. Dùng lịch sử Git có sẵn của source xv6 để push, không upload từng file hoặc xóa `.git` rồi khởi tạo lại.
- [ ] Trên bản source của Khang, giữ remote MIT với tên `upstream`, đặt `origin` là repo GitHub của nhóm; kiểm tra remote hiện tại trước khi đổi. Phú/Phúc clone repo nhóm thì origin tự trỏ tới GitHub nhóm.
- [ ] Tạo `main` từ SHA syscall ở mục 2; đặt tag `lab0-base` tại SHA đó và push tag lên repo nhóm. Đặt main làm nhánh mặc định.
- [ ] Giữ nguyên cấu trúc source và nội dung README. Nếu thêm kế hoạch vào `docs/Ke_hoach_Lab0.md`, dùng commit tài liệu riêng sau tag baseline và trước khi tạo các nhánh tính năng. Không cần thêm tài liệu khác chỉ để đủ quy trình.
- [ ] Tạo và push cả ba nhánh tính năng từ cùng main ban đầu. Không cần đưa toàn bộ nhánh bài lab khác từ MIT lên repo nhóm.
- [ ] Gửi URL repo và tên nhánh cho hai bạn; sau bước khởi tạo này, không cần chờ nhau để làm việc.

Tài liệu nhóm được commit sau baseline sẽ xuất hiện trong diff/source archive cuối nếu xuất toàn bộ HEAD; điều này không phải đóng góp code. Khi rà đóng góp, xem riêng các file source được phân công.

### 8.2. Tên và quyền sử dụng nhánh

| Nhánh / tag | Người phụ trách | Vai trò |
|---|---|---|
| `main` | Khang quản lý tích hợp | Nhận PR; bản cuối dùng để kiểm thử và xuất gói nộp |
| `feat/trace-user` | Phú | User program, prototype, stub, Makefile |
| `feat/trace-handler` | Phúc | Mask process, handler và số syscall |
| `feat/trace-dispatcher` | Khang | Đăng ký handler, bảng tên và in trace |
| `fix/trace-integration` | Khang | Chỉ tạo từ main đã tích hợp khi cần sửa lỗi cuối |
| `lab0-base` (tag) | Khang tạo một lần | Mốc source gốc cố định để tạo patch nộp bài |

Sau khởi tạo, nhóm thống nhất không push trực tiếp main, không push vào nhánh người khác và không force-push lịch sử đã chia sẻ. Các thay đổi đi qua PR, Khang là người merge. Đây là quy ước phối hợp; không cần cài thêm CI hay công cụ quản lý.

### 8.3. Mỗi người làm và bàn giao độc lập

1. Clone repo nhóm, checkout nhánh được giao và dùng tên/email Git của chính mình.
2. Sửa đúng file, tự kiểm tra theo phần việc, commit thay đổi có ý nghĩa rồi push nhánh của mình. Chỉ add các file cần thiết, không add file build hoặc ZIP nộp bài.
3. Khi phần việc đã sẵn sàng, mở **Pull Request với base là main**. Có thể mở Draft PR sớm để Khang xem tiến độ; đổi sang sẵn sàng review khi hoàn tất phần riêng.
4. Nội dung PR theo mẫu ngắn:

```text
Phần đã làm:
File đã sửa:
Đã kiểm tra: lệnh và kết quả thực tế
Chưa kiểm tra / lỗi còn biết:
Phụ thuộc giao diện: theo mục 2 của kế hoạch
Lưu ý khi tích hợp:
```

5. Khang review diff và giao diện, trao đổi ngay trên PR nếu cần sửa. Tác giả push commit sửa tiếp lên cùng nhánh thì PR được cập nhật.
6. Không cần merge nhánh của bạn khác vào nhánh mình để hoàn thành phần riêng. Ghi rõ giới hạn build do thiếu thành phần đã dự kiến, không đánh dấu toàn luồng đã pass.

Phú và Phúc không phải chờ PR của nhau được merge mới mở PR. Phú và Phúc cùng làm báo cáo; nếu lưu nội dung báo cáo trong repo thì dùng commit tài liệu riêng, thống nhất người ghép và không sửa trùng file. Không trộn tài liệu vào phần code chỉ để tăng số dòng đóng góp.

### 8.4. Khang tích hợp và sửa cuối

- Khang có thể review/merge phần nào đến trước. Chọn **Create a merge commit** để giữ các commit của tác giả; không squash hoặc rebase khi merge PR của nhóm.
- Main trong giai đoạn mới ghép một phần có thể chưa build được vì thiếu các phần còn lại. Ghi rõ tình trạng; main chỉ được coi là bản sẵn sàng nộp khi ghép đủ, sửa lỗi và đạt T1–T3.
- Sau khi merge đủ ba PR, Khang cập nhật main ở máy mình, build/test theo mục 7. Nếu có lỗi, dùng `fix/trace-integration`, kiểm thử trên nhánh fix rồi mở PR và merge vào main.
- Sau khi PR đã merge, mọi sửa tiếp phải thống nhất với Khang để tránh hai người sửa cùng file. Không tự tiếp tục thay đổi rồi cho rằng bản sửa đã có trong main.
- Nếu cần đưa cập nhật main vào một nhánh còn làm dở, fetch rồi merge `origin/main` vào nhánh đó khi working tree sạch; không dùng reset/force-push để bỏ qua xung đột.

### 8.5. Ghi nhận đóng góp và patch nộp bài

Mỗi người có commit riêng bằng danh tính của mình; PR thể hiện phạm vi và kiểm tra; báo cáo ghi đúng đóng góp thực tế. Khang giữ riêng commit sửa lỗi cuối. Không ép số dòng bằng nhau hoặc viết lại code có sẵn để làm đẹp thống kê.

GitHub thay việc gửi patch qua lại trong nhóm. **Diff patch nộp Moodle vẫn phải tạo từ `lab0-base` tới main cuối theo mục 7.** Diff patch không ghi tác giả từng dòng; lịch sử Git và phân công báo cáo giúp đối chiếu công sức. Không cần nộp thêm lịch sử Git nếu đề không yêu cầu.

## 9. Mốc thời gian linh hoạt

Gọi **D là hạn nộp thực tế của lớp**; chưa có ngày cụ thể trong tài liệu.

| Mốc đề xuất | Phú | Phúc | Khang |
|---|---|---|---|
| Bất kỳ lúc nào trước bàn giao | Tự chuẩn bị, code user, kiểm tra object, viết phần báo cáo | Tự chuẩn bị, code mask/handler, kiểm tra object, viết phần báo cáo | Code dispatcher, chuẩn bị bảng test và bản tích hợp |
| Chậm nhất D−2 ngày | Push và mở PR user vào main; không đợi Phúc | Push và mở PR handler vào main; không đợi Phú | Mở PR dispatcher, review/merge từng PR rồi kiểm thử khi đủ |
| D−2 đến D−1 | Viết phần user, bố cục; đọc chéo phần Phúc | Viết kernel/bảng test, ghép nội dung và xuất PDF | Sửa lỗi cuối, xác nhận T1–T3; gửi log và ghi chú kỹ thuật |
| Trước D | Kiểm tra hiển thị và cùng duyệt PDF | Kiểm tra nội dung, cùng chốt và gửi PDF | Tạo/kiểm tra patch, source ZIP; đóng gói, nộp Moodle và xác nhận |

D−2 là khoảng đệm nhóm đề xuất, không phải hạn do đề quy định. Nếu thời gian còn ít thì rút ngắn mốc, vẫn dành khoảng riêng cho Khang tích hợp. Không ai phải chờ một cuộc họp hoặc người khác online để bắt đầu phần code.

## 10. Checklist hoàn thành toàn nhóm

- [ ] Repo Private có đủ ba phần code qua PR, đúng nhánh và ranh giới file; giữ rõ commit tác giả và commit sửa lỗi cuối.
- [ ] Main đã chứa mọi PR cần nộp; tag `lab0-base` vẫn trỏ đúng source gốc.
- [ ] Source tích hợp build được và T1–T3 đạt; không PID, không kế thừa mask qua fork, không sysinfo.
- [ ] PDF do Phú và Phúc cùng hoàn thiện, đã kiểm tra chéo; ngắn, trung thực, tên file đúng MSSV tăng dần.
- [ ] Khang đã tạo và kiểm tra patch đủ thay đổi từ baseline, source sạch và gói nộp đầy đủ.
- [ ] Khang đã nộp Moodle và xác nhận nhận bài.
- [ ] Phú hiểu user/argv/stub/exec; Phúc hiểu argint/mask/vòng đời process; Khang hiểu dispatcher/bit mask/return value và kiểm thử.
- [ ] Cả ba đọc phần của nhau để giải thích luồng syscall trong phỏng vấn cá nhân như đề yêu cầu.

Thông tin còn cần điền khi có: MSSV từng người, hạn nộp và lịch demo. Giao diện kỹ thuật và phân công đã chốt, không cần đợi thêm để bắt đầu.
