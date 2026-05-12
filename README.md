# Graph Simulator — Đỉnh Trụ & Cạnh Cầu

Trình mô phỏng thuật toán tìm **Articulation Points (Đỉnh Trụ)** và **Bridges (Cạnh Cầu)** bằng C++ + SDL2.

---

## Yêu cầu

| Thành phần | Tải về |
|---|---|
| MinGW-w64 (g++) | https://winlibs.com hoặc https://www.msys2.org |
| SDL2 (runtime + dev) | https://github.com/libsdl-org/SDL/releases |
| SDL2_ttf (runtime + dev) | https://github.com/libsdl-org/SDL_ttf/releases |

---

## Cài đặt nhanh (MSYS2 — dễ nhất)

```bash
pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-gcc
```

Sau đó thêm `C:\msys64\mingw64\bin` vào PATH.

---

## Build & Chạy

1. Mở **Command Prompt** hoặc **PowerShell** trong thư mục dự án.
2. Chạy:
   ```
   build.bat
   ```
3. File `GraphSimulator.exe` được tạo ra và tự động khởi chạy.

> **Lưu ý:** Nếu SDL2 của bạn không nằm ở đường dẫn mặc định, mở `build.bat`  
> và sửa biến `SDL2_DIR` cho đúng thư mục gốc SDL2.

---

## Hướng dẫn sử dụng

| Chế độ | Thao tác |
|---|---|
| **Thêm đỉnh** | Click vào vùng trống trong đồ thị |
| **Thêm cạnh** | Click đỉnh 1 → Click đỉnh 2 |
| **Xoá đỉnh** | Click vào đỉnh cần xoá |
| **Xoá cạnh** | Click gần cạnh cần xoá |
| **Di chuyển đỉnh** | Kéo thả đỉnh |
| **Xoá tất cả** | Nút "Xoá tất cả" |

### Chú thích màu

- 🔴 **Vòng đỏ quanh đỉnh** = Đỉnh Trụ (Articulation Point)
- 🟡 **Cạnh vàng-cam đậm** = Cạnh Cầu (Bridge)
- Mỗi **thành phần liên thông** được tô một màu khác nhau

---

## Thuật toán

Sử dụng **DFS Tarjan** với `disc[]` và `low[]`:

- **Đỉnh trụ**: `low[v] >= disc[u]` (với `u` không phải gốc) hoặc gốc có ≥ 2 con DFS.
- **Cạnh cầu**: `low[v] > disc[u]`.
- **Thành phần liên thông**: DFS flood-fill.

Độ phức tạp: **O(V + E)** mỗi lần cập nhật.
