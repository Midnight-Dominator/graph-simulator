#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <algorithm>
#include <string>
#include <cmath>
#include <map>
#include <set>
#include <climits>

// ─── Layout ───────────────────────────────────────────────────────────────────
const int WINDOW_W = 1480;
const int WINDOW_H = 780;
const int SIDEBAR_W = 260; // left tool panel
const int TOPBAR_H = 56;
const int BOTTOM_H = 240; // bottom info panels
const int NODE_RADIUS = 22;

// Bottom panel X positions (3 equal panels)
const int BP_Y = WINDOW_H - BOTTOM_H;
const int BP1_X = SIDEBAR_W;
const int BP2_X = SIDEBAR_W + (WINDOW_W - SIDEBAR_W) / 3;
const int BP3_X = SIDEBAR_W + 2 * (WINDOW_W - SIDEBAR_W) / 3;
const int BP_W = (WINDOW_W - SIDEBAR_W) / 3;

// Graph canvas
const int GRAPH_X = SIDEBAR_W;
const int GRAPH_Y = TOPBAR_H;
const int GRAPH_W = WINDOW_W - SIDEBAR_W;
const int GRAPH_H = WINDOW_H - TOPBAR_H - BOTTOM_H;

// ─── Theme ────────────────────────────────────────────────────────────────────
struct Theme
{
    SDL_Color bg, panel, panel2, border;
    SDL_Color text, textDim, accent;
    SDL_Color edgeNorm;
    SDL_Color btnNorm, btnHover;
    bool isDark;
};

Theme darkTheme()
{
    return {
        {13, 15, 24, 255}, {20, 23, 36, 255}, {26, 30, 46, 255}, {42, 50, 78, 255}, {208, 213, 238, 255}, {105, 115, 155, 255}, {80, 140, 255, 255}, {55, 65, 95, 255}, {28, 33, 55, 255}, {42, 52, 88, 255}, true};
}
Theme lightTheme()
{
    return {
        {230, 233, 243, 255}, {252, 253, 255, 255}, {240, 243, 252, 255}, {185, 195, 220, 255}, {22, 28, 52, 255}, {118, 128, 158, 255}, {42, 92, 210, 255}, {138, 153, 188, 255}, {213, 218, 236, 255}, {193, 203, 228, 255}, false};
}

const SDL_Color COL_ARTPT = {255, 68, 92, 255};
const SDL_Color COL_BRIDGE = {255, 175, 30, 255};
const SDL_Color COL_DELETE = {178, 38, 58, 255};
const SDL_Color COL_DELETE_H = {218, 58, 78, 255};
const SDL_Color COL_BTN_ACT = {48, 98, 208, 255};
const SDL_Color COL_BTN_HOV = {33, 68, 158, 255};
const SDL_Color COL_GREEN = {52, 192, 112, 255};
const SDL_Color COL_BRIDGE_ROW = {80, 50, 10, 255};      // edge list bridge row bg (dark)
const SDL_Color COL_BRIDGE_ROW_L = {255, 240, 200, 255}; // light mode

const SDL_Color COMP_COLORS[] = {
    {100, 178, 255, 255}, {78, 218, 128, 255}, {255, 193, 68, 255}, {208, 93, 255, 255}, {68, 213, 213, 255}, {255, 123, 68, 255}, {148, 255, 88, 255}, {255, 68, 153, 255}, {68, 153, 255, 255}, {193, 255, 68, 255}};
const int NUM_CC = 10;

// ─── Data ─────────────────────────────────────────────────────────────────────
struct Node
{
    int id;
    float x, y;
    bool isAP = false;
    int comp = -1;
};
struct Edge
{
    int u, v;
    bool isBridge = false;
};

std::vector<Node> nodes;
std::vector<Edge> edges;
int nextId = 1;

// ─── Tarjan ───────────────────────────────────────────────────────────────────
int dfsT = 0;
std::vector<int> disc2, low2, par2;
std::vector<bool> vis2;

void dfs(int u, std::vector<std::vector<int>> &adj)
{
    vis2[u] = true;
    disc2[u] = low2[u] = ++dfsT;
    int ch = 0;
    for (int v : adj[u])
    {
        if (!vis2[v])
        {
            ch++;
            par2[v] = u;
            dfs(v, adj);
            low2[u] = std::min(low2[u], low2[v]);
            if (par2[u] == -1 && ch > 1)
                nodes[u].isAP = true;
            if (par2[u] != -1 && low2[v] >= disc2[u])
                nodes[u].isAP = true;
            if (low2[v] > disc2[u])
                for (auto &e : edges)
                    if ((e.u == nodes[u].id && e.v == nodes[v].id) || (e.u == nodes[v].id && e.v == nodes[u].id))
                        e.isBridge = true;
        }
        else if (v != par2[u])
            low2[u] = std::min(low2[u], disc2[v]);
    }
}

void analyzeGraph()
{
    int n = (int)nodes.size();
    for (auto &nd : nodes)
    {
        nd.isAP = false;
        nd.comp = -1;
    }
    for (auto &e : edges)
        e.isBridge = false;
    if (!n)
        return;
    std::map<int, int> idx;
    for (int i = 0; i < n; i++)
        idx[nodes[i].id] = i;
    std::vector<std::vector<int>> adj(n);
    for (auto &e : edges)
        if (idx.count(e.u) && idx.count(e.v))
        {
            int ui = idx[e.u], vi = idx[e.v];
            adj[ui].push_back(vi);
            adj[vi].push_back(ui);
        }
    disc2.assign(n, 0);
    low2.assign(n, 0);
    par2.assign(n, -1);
    vis2.assign(n, false);
    dfsT = 0;
    for (int i = 0; i < n; i++)
        if (!vis2[i])
            dfs(i, adj);
    // components
    std::vector<bool> asgn(n, false);
    int cid = 0;
    for (int i = 0; i < n; i++)
        if (!asgn[i])
        {
            std::vector<int> stk = {i};
            asgn[i] = true;
            nodes[i].comp = cid;
            while (!stk.empty())
            {
                int cur = stk.back();
                stk.pop_back();
                for (int nb : adj[cur])
                    if (!asgn[nb])
                    {
                        asgn[nb] = true;
                        nodes[nb].comp = cid;
                        stk.push_back(nb);
                    }
            }
            cid++;
        }
}

int countComps()
{
    if (nodes.empty())
        return 0;
    std::set<int> s;
    for (auto &nd : nodes)
        s.insert(nd.comp);
    return (int)s.size();
}

Node *findById(int id)
{
    for (auto &nd : nodes)
        if (nd.id == id)
            return &nd;
    return nullptr;
}
Node *findAt(float mx, float my)
{
    for (auto &nd : nodes)
    {
        float dx = nd.x - mx, dy = nd.y - my;
        if (dx * dx + dy * dy <= (float)(NODE_RADIUS * NODE_RADIUS))
            return &nd;
    }
    return nullptr;
}
bool edgeEx(int u, int v)
{
    for (auto &e : edges)
        if ((e.u == u && e.v == v) || (e.u == v && e.v == u))
            return true;
    return false;
}

// ─── SDL helpers ──────────────────────────────────────────────────────────────
void sc(SDL_Renderer *r, SDL_Color c) { SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a); }
void fr(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color c)
{
    sc(r, c);
    SDL_Rect rc = {x, y, w, h};
    SDL_RenderFillRect(r, &rc);
}
void dr(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color c)
{
    sc(r, c);
    SDL_Rect rc = {x, y, w, h};
    SDL_RenderDrawRect(r, &rc);
}
void dl(SDL_Renderer *r, int x1, int y1, int x2, int y2, SDL_Color c)
{
    sc(r, c);
    SDL_RenderDrawLine(r, x1, y1, x2, y2);
}

void thickLine(SDL_Renderer *r, int x1, int y1, int x2, int y2, SDL_Color c, int t)
{
    float dx = (float)(x2 - x1), dy = (float)(y2 - y1), len = sqrtf(dx * dx + dy * dy);
    if (len < 1)
        return;
    for (int i = -t / 2; i <= t / 2; i++)
    {
        float nx = -dy / len * i, ny = dx / len * i;
        sc(r, c);
        SDL_RenderDrawLine(r, x1 + (int)nx, y1 + (int)ny, x2 + (int)nx, y2 + (int)ny);
    }
}
void circle(SDL_Renderer *r, int cx, int cy, int rad, SDL_Color c)
{
    sc(r, c);
    int x = rad, y = 0, e = 0;
    while (x >= y)
    {
        SDL_RenderDrawPoint(r, cx + x, cy + y);
        SDL_RenderDrawPoint(r, cx + y, cy + x);
        SDL_RenderDrawPoint(r, cx - y, cy + x);
        SDL_RenderDrawPoint(r, cx - x, cy + y);
        SDL_RenderDrawPoint(r, cx - x, cy - y);
        SDL_RenderDrawPoint(r, cx - y, cy - x);
        SDL_RenderDrawPoint(r, cx + y, cy - x);
        SDL_RenderDrawPoint(r, cx + x, cy - y);
        y++;
        if (e <= 0)
            e += 2 * y + 1;
        else
        {
            x--;
            e += 2 * (y - x) + 1;
        }
    }
}
void disk(SDL_Renderer *r, int cx, int cy, int rad, SDL_Color c)
{
    sc(r, c);
    for (int dy = -rad; dy <= rad; dy++)
    {
        int dx = (int)sqrtf((float)(rad * rad - dy * dy));
        SDL_RenderDrawLine(r, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}
void text(SDL_Renderer *r, TTF_Font *f, const std::string &s, int x, int y, SDL_Color c, bool cen = false)
{
    if (s.empty())
        return;
    SDL_Surface *su = TTF_RenderUTF8_Blended(f, s.c_str(), c);
    if (!su)
        return;
    SDL_Texture *tx = SDL_CreateTextureFromSurface(r, su);
    SDL_Rect dst = {x, y, su->w, su->h};
    if (cen)
    {
        dst.x = x - su->w / 2;
        dst.y = y - su->h / 2;
    }
    SDL_RenderCopy(r, tx, nullptr, &dst);
    SDL_DestroyTexture(tx);
    SDL_FreeSurface(su);
}
// clipped text render
void textClip(SDL_Renderer *r, TTF_Font *f, const std::string &s, int x, int y, SDL_Color c, SDL_Rect clip)
{
    if (s.empty())
        return;
    SDL_Surface *su = TTF_RenderUTF8_Blended(f, s.c_str(), c);
    if (!su)
        return;
    SDL_Texture *tx = SDL_CreateTextureFromSurface(r, su);
    SDL_Rect dst = {x, y, su->w, su->h};
    // intersect dst with clip
    int x0 = std::max(dst.x, clip.x), y0 = std::max(dst.y, clip.y);
    int x1 = std::min(dst.x + dst.w, clip.x + clip.w), y1 = std::min(dst.y + dst.h, clip.y + clip.h);
    if (x1 > x0 && y1 > y0)
    {
        SDL_Rect src = {x0 - dst.x, y0 - dst.y, x1 - x0, y1 - y0};
        SDL_Rect d2 = {x0, y0, x1 - x0, y1 - y0};
        SDL_RenderCopy(r, tx, &src, &d2);
    }
    SDL_DestroyTexture(tx);
    SDL_FreeSurface(su);
}
void rrect(SDL_Renderer *r, int x, int y, int w, int h, int rad, SDL_Color c)
{
    fr(r, x + rad, y, w - 2 * rad, h, c);
    fr(r, x, y + rad, rad, h - 2 * rad, c);
    fr(r, x + w - rad, y + rad, rad, h - 2 * rad, c);
    disk(r, x + rad, y + rad, rad, c);
    disk(r, x + w - rad, y + rad, rad, c);
    disk(r, x + rad, y + h - rad, rad, c);
    disk(r, x + w - rad, y + h - rad, rad, c);
}

// ─── Button ───────────────────────────────────────────────────────────────────
struct Btn
{
    int x, y, w, h;
    std::string lbl;
    SDL_Color color, hov;
    bool active = false;
    bool has(int mx, int my) const { return mx >= x && mx <= x + w && my >= y && my <= y + h; }
    void draw(SDL_Renderer *r, TTF_Font *f, int mx, int my, const Theme &th) const
    {
        SDL_Color col = active ? color : (has(mx, my) ? hov : th.btnNorm);
        rrect(r, x, y, w, h, 6, col);
        SDL_Color bc = active ? color : th.border;
        sc(r, bc);
        SDL_Rect rc = {x, y, w, h};
        SDL_RenderDrawRect(r, &rc);
        SDL_Color tc = active ? SDL_Color{255, 255, 255, 255} : th.text;
        SDL_Surface *s = TTF_RenderUTF8_Blended(f, lbl.c_str(), tc);
        if (s)
        {
            SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
            SDL_Rect d = {x + w / 2 - s->w / 2, y + h / 2 - s->h / 2, s->w, s->h};
            SDL_RenderCopy(r, t, nullptr, &d);
            SDL_DestroyTexture(t);
            SDL_FreeSurface(s);
        }
    }
};

struct ThemeBtn
{
    int x, y, w, h;
    bool has(int mx, int my) const { return mx >= x && mx <= x + w && my >= y && my <= y + h; }
    void draw(SDL_Renderer *r, TTF_Font *f, int mx, int my, bool isDark, const Theme &th) const
    {
        SDL_Color bg = has(mx, my) ? th.btnHover : th.btnNorm;
        rrect(r, x, y, w, h, 8, bg);
        sc(r, th.border);
        SDL_Rect rc = {x, y, w, h};
        SDL_RenderDrawRect(r, &rc);
        std::string icon = isDark ? "☀ Sang" : "☾ Toi";
        text(r, f, icon, x + 8, y + h / 2 - 8, th.text);
    }
};

// ─── Scrollable list state ────────────────────────────────────────────────────
struct ScrollList
{
    int scrollY = 0;
};
ScrollList edgeScroll, matScroll;

enum Mode
{
    ADD_NODE,
    ADD_EDGE,
    DEL_NODE,
    DEL_EDGE,
    DRAG
};

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(int, char **)
{
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window *win = SDL_CreateWindow("Graph Simulator - Articulation Points & Bridges",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    TTF_Font *fLg = nullptr, *fMd = nullptr, *fSm = nullptr, *fNd = nullptr, *fMono = nullptr;
    const char *fp[] = {"C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf",
                        "C:/Windows/Fonts/calibri.ttf",
                        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", nullptr};
    const char *fm[] = {"C:/Windows/Fonts/consola.ttf", "C:/Windows/Fonts/cour.ttf",
                        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", nullptr};
    for (int i = 0; fp[i] && !fLg; i++)
    {
        fLg = TTF_OpenFont(fp[i], 20);
        fMd = TTF_OpenFont(fp[i], 15);
        fSm = TTF_OpenFont(fp[i], 13);
        fNd = TTF_OpenFont(fp[i], 18);
    }
    for (int i = 0; fm[i] && !fMono; i++)
        fMono = TTF_OpenFont(fm[i], 13);
    if (!fMono && fSm)
        fMono = fSm;
    if (!fLg)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "No font found.", win);
        return 1;
    }

    bool isDark = true;
    Theme th = darkTheme();
    Mode mode = ADD_NODE;
    int edgeStart = -1, dragNode = -1;
    bool running = true;
    SDL_Event ev;
    std::string statusMsg = "Click vao vung do thi de them dinh.";

    // ── Build sidebar buttons ──
    int bx = 8, by = TOPBAR_H + 30, bw = SIDEBAR_W - 16, bh = 38, bg = 7;
    auto mkBtn = [&](const std::string &l, SDL_Color c, SDL_Color h) -> Btn
    {
        Btn b;
        b.x = bx;
        b.y = by;
        b.w = bw;
        b.h = bh;
        b.lbl = l;
        b.color = c;
        b.hov = h;
        by += bh + bg;
        return b;
    };
    Btn bAddN = mkBtn("[ + ] Them dinh", COL_BTN_ACT, COL_BTN_HOV);
    Btn bAddE = mkBtn("[ + ] Them canh", COL_BTN_ACT, COL_BTN_HOV);
    Btn bDelN = mkBtn("[ - ] Xoa dinh", COL_DELETE, COL_DELETE_H);
    Btn bDelE = mkBtn("[ - ] Xoa canh", COL_DELETE, COL_DELETE_H);
    Btn bDrag = mkBtn("[ >> ] Di chuyen", {48, 88, 52, 255}, {62, 118, 62, 255});
    Btn bClr = mkBtn("[ X ] Xoa tat ca", {68, 28, 28, 255}, {98, 43, 43, 255});
    int sideEnd = by + 6;

    ThemeBtn bTheme;
    bTheme.x = WINDOW_W - 108;
    bTheme.y = 10;
    bTheme.w = 96;
    bTheme.h = 34;

    auto setMode = [&](Mode m)
    {
        mode = m;
        edgeStart = -1;
        bAddN.active = (m == ADD_NODE);
        bAddE.active = (m == ADD_EDGE);
        bDelN.active = (m == DEL_NODE);
        bDelE.active = (m == DEL_EDGE);
        bDrag.active = (m == DRAG);
        switch (m)
        {
        case ADD_NODE:
            statusMsg = "Click vao vung do thi de them dinh.";
            break;
        case ADD_EDGE:
            statusMsg = "Click chon 2 dinh de them canh.";
            break;
        case DEL_NODE:
            statusMsg = "Click vao dinh de xoa.";
            break;
        case DEL_EDGE:
            statusMsg = "Click gan canh de xoa.";
            break;
        case DRAG:
            statusMsg = "Keo dinh de di chuyen.";
            break;
        }
    };
    bAddN.active = true;

    // Matrix scroll offset (horizontal)
    int matScrollX = 0, matScrollY = 0;
    int edgeScrollY = 0;
    const int ROW_H = 24;
    const int MAT_CELL = 28;

    while (running)
    {
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
                running = false;
            if (ev.type == SDL_KEYDOWN)
            {
                if (ev.key.keysym.sym == SDLK_ESCAPE)
                    running = false;
                if (ev.key.keysym.sym == SDLK_t)
                {
                    isDark = !isDark;
                    th = isDark ? darkTheme() : lightTheme();
                }
            }

            // ── Scroll wheel for bottom panels ──
            if (ev.type == SDL_MOUSEWHEEL)
            {
                int wx = ev.wheel.x > 0 ? 1 : (ev.wheel.x < 0 ? -1 : 0);
                int wy = ev.wheel.y > 0 ? -1 : (ev.wheel.y < 0 ? 1 : 0);
                // panel 2 (edge list): BP2_X .. BP3_X, BP_Y .. WINDOW_H
                if (mx >= BP2_X && mx < BP3_X && my >= BP_Y)
                {
                    edgeScrollY = std::max(0, edgeScrollY + wy * ROW_H * 3);
                }
                // panel 3 (matrix): BP3_X .. WINDOW_W, BP_Y .. WINDOW_H
                if (mx >= BP3_X && my >= BP_Y)
                {
                    matScrollY = std::max(0, matScrollY + wy * MAT_CELL);
                    matScrollX = std::max(0, matScrollX + wx * MAT_CELL);
                }
            }

            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT)
            {
                int cx = ev.button.x, cy = ev.button.y;
                if (bTheme.has(cx, cy))
                {
                    isDark = !isDark;
                    th = isDark ? darkTheme() : lightTheme();
                    continue;
                }
                if (bAddN.has(cx, cy))
                {
                    setMode(ADD_NODE);
                    continue;
                }
                if (bAddE.has(cx, cy))
                {
                    setMode(ADD_EDGE);
                    continue;
                }
                if (bDelN.has(cx, cy))
                {
                    setMode(DEL_NODE);
                    continue;
                }
                if (bDelE.has(cx, cy))
                {
                    setMode(DEL_EDGE);
                    continue;
                }
                if (bDrag.has(cx, cy))
                {
                    setMode(DRAG);
                    continue;
                }
                if (bClr.has(cx, cy))
                {
                    nodes.clear();
                    edges.clear();
                    nextId = 1;
                    edgeStart = -1;
                    edgeScrollY = 0;
                    matScrollX = matScrollY = 0;
                    statusMsg = "Da xoa tat ca.";
                    continue;
                }
                // only graph canvas area
                if (cx < SIDEBAR_W || cy < TOPBAR_H || cy >= BP_Y)
                    continue;
                float gx = (float)cx, gy = (float)cy;
                if (mode == ADD_NODE)
                {
                    if (!findAt(gx, gy))
                    {
                        Node nd;
                        nd.id = nextId++;
                        nd.x = gx;
                        nd.y = gy;
                        nodes.push_back(nd);
                        analyzeGraph();
                        statusMsg = "Da them dinh " + std::to_string(nd.id) + ".";
                    }
                }
                else if (mode == ADD_EDGE)
                {
                    Node *h = findAt(gx, gy);
                    if (h)
                    {
                        if (edgeStart == -1)
                        {
                            edgeStart = h->id;
                            statusMsg = "Da chon dinh " + std::to_string(edgeStart) + ". Click dinh thu 2.";
                        }
                        else
                        {
                            if (h->id != edgeStart && !edgeEx(edgeStart, h->id))
                            {
                                edges.push_back({edgeStart, h->id, false});
                                analyzeGraph();
                                statusMsg = "Da them canh (" + std::to_string(edgeStart) + "," + std::to_string(h->id) + ").";
                            }
                            else
                                statusMsg = "Canh da ton tai hoac dinh trung.";
                            edgeStart = -1;
                        }
                    }
                }
                else if (mode == DEL_NODE)
                {
                    Node *h = findAt(gx, gy);
                    if (h)
                    {
                        int id = h->id;
                        nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [id](const Node &n)
                                                   { return n.id == id; }),
                                    nodes.end());
                        edges.erase(std::remove_if(edges.begin(), edges.end(), [id](const Edge &e)
                                                   { return e.u == id || e.v == id; }),
                                    edges.end());
                        analyzeGraph();
                        statusMsg = "Da xoa dinh " + std::to_string(id) + ".";
                        edgeScrollY = 0;
                        matScrollX = matScrollY = 0;
                    }
                }
                else if (mode == DEL_EDGE)
                {
                    float best = 14.0f;
                    int bi = -1;
                    for (int i = 0; i < (int)edges.size(); i++)
                    {
                        Node *na = findById(edges[i].u), *nb = findById(edges[i].v);
                        if (!na || !nb)
                            continue;
                        float ax = na->x, ay = na->y, bx2 = nb->x, by2 = nb->y, dx = bx2 - ax, dy2 = by2 - ay;
                        float t = ((gx - ax) * dx + (gy - ay) * dy2) / (dx * dx + dy2 * dy2 + 0.001f);
                        t = std::max(0.0f, std::min(1.0f, t));
                        float px = ax + t * dx, py = ay + t * dy2, dist = sqrtf((gx - px) * (gx - px) + (gy - py) * (gy - py));
                        if (dist < best)
                        {
                            best = dist;
                            bi = i;
                        }
                    }
                    if (bi >= 0)
                    {
                        int u = edges[bi].u, v = edges[bi].v;
                        edges.erase(edges.begin() + bi);
                        analyzeGraph();
                        statusMsg = "Da xoa canh (" + std::to_string(u) + "," + std::to_string(v) + ").";
                    }
                }
                else if (mode == DRAG)
                {
                    Node *h = findAt(gx, gy);
                    if (h)
                        dragNode = h->id;
                }
            }
            if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT)
                if (dragNode != -1)
                {
                    dragNode = -1;
                    analyzeGraph();
                }
            if (ev.type == SDL_MOUSEMOTION && dragNode != -1 && mode == DRAG)
            {
                Node *nd = findById(dragNode);
                if (nd)
                {
                    // clamp to graph area
                    nd->x = std::max((float)(SIDEBAR_W + NODE_RADIUS), std::min((float)(WINDOW_W - NODE_RADIUS), (float)ev.motion.x));
                    nd->y = std::max((float)(TOPBAR_H + NODE_RADIUS), std::min((float)(BP_Y - NODE_RADIUS), (float)ev.motion.y));
                }
            }
        }

        // ════════════════════════════════════════════════════════════════════
        // RENDER
        // ════════════════════════════════════════════════════════════════════
        sc(ren, th.bg);
        SDL_RenderClear(ren);

        // ── Top bar ──────────────────────────────────────────────────────────
        fr(ren, 0, 0, WINDOW_W, TOPBAR_H, th.panel);
        dl(ren, 0, TOPBAR_H - 1, WINDOW_W, TOPBAR_H - 1, th.border);
        text(ren, fLg, "GRAPH SIMULATOR", 14, 7, th.accent);
        text(ren, fSm, "Articulation Points & Bridges", 14, 31, th.textDim);
        text(ren, fMd, statusMsg, SIDEBAR_W + 12, 19, th.text);
        bTheme.draw(ren, fSm, mx, my, isDark, th);

        // ── Sidebar ──────────────────────────────────────────────────────────
        fr(ren, 0, TOPBAR_H, SIDEBAR_W, WINDOW_H - TOPBAR_H, th.panel);
        dl(ren, SIDEBAR_W, TOPBAR_H, SIDEBAR_W, WINDOW_H, th.border);
        text(ren, fSm, "CONG CU", 10, TOPBAR_H + 10, th.textDim);
        bAddN.draw(ren, fMd, mx, my, th);
        bAddE.draw(ren, fMd, mx, my, th);
        bDelN.draw(ren, fMd, mx, my, th);
        bDelE.draw(ren, fMd, mx, my, th);
        bDrag.draw(ren, fMd, mx, my, th);
        bClr.draw(ren, fMd, mx, my, th);

        // ── Stats in sidebar ──
        int sepY = sideEnd;
        fr(ren, 8, sepY, SIDEBAR_W - 16, 1, th.border);
        int sY = sepY + 10;
        text(ren, fSm, "THONG KE", 10, sY, th.textDim);
        sY += 22;

        int apCnt = 0, brCnt = 0;
        for (auto &nd : nodes)
            if (nd.isAP)
                apCnt++;
        for (auto &e : edges)
            if (e.isBridge)
                brCnt++;
        int cc = countComps();

        auto stat = [&](const std::string &l, const std::string &v, SDL_Color vc)
        {
            text(ren, fMd, l, 10, sY, th.textDim);
            text(ren, fMd, v, SIDEBAR_W - 8 - (int)v.size() * 10, sY, vc);
            sY += 22;
        };
        stat("So dinh:", std::to_string(nodes.size()), th.text);
        stat("So canh:", std::to_string(edges.size()), th.text);
        fr(ren, 8, sY, SIDEBAR_W - 16, 1, th.border);
        sY += 8;
        stat("TPLT:", std::to_string(cc), COL_GREEN);
        stat("Dinh tru:", std::to_string(apCnt), COL_ARTPT);
        stat("Canh cau:", std::to_string(brCnt), COL_BRIDGE);

        // ── Legend ──
        fr(ren, 8, sY, SIDEBAR_W - 16, 1, th.border);
        sY += 10;
        text(ren, fSm, "CHU THICH", 10, sY, th.textDim);
        sY += 20;
        auto ldot = [&](SDL_Color c, const std::string &l)
        {
            disk(ren, 20, sY + 8, 8, c);
            circle(ren, 20, sY + 8, 8, {0, 0, 0, 25});
            text(ren, fSm, l, 35, sY + 1, th.text);
            sY += 22;
        };
        ldot(COMP_COLORS[0], "Dinh thuong");
        ldot(COL_ARTPT, "Dinh tru (AP)");
        fr(ren, 10, sY + 5, 28, 4, th.edgeNorm);
        text(ren, fSm, "Canh thuong", 44, sY, th.text);
        sY += 20;
        fr(ren, 10, sY + 5, 28, 5, COL_BRIDGE);
        text(ren, fSm, "Canh cau (Bridge)", 44, sY, th.text);
        sY += 22;

        text(ren, fSm, "[T] doi giao dien", 8, WINDOW_H - 20, th.textDim);

        // ── Graph canvas background ──
        fr(ren, SIDEBAR_W, TOPBAR_H, GRAPH_W, GRAPH_H, th.bg);

        // ── Draw edges on canvas ──
        for (auto &e : edges)
        {
            Node *na = findById(e.u), *nb = findById(e.v);
            if (!na || !nb)
                continue;
            SDL_Color ec = e.isBridge ? COL_BRIDGE : th.edgeNorm;
            thickLine(ren, (int)na->x, (int)na->y, (int)nb->x, (int)nb->y, ec, e.isBridge ? 5 : 2);
        }
        if (mode == ADD_EDGE && edgeStart != -1)
        {
            Node *na = findById(edgeStart);
            if (na)
            {
                sc(ren, {100, 160, 255, 90});
                SDL_RenderDrawLine(ren, (int)na->x, (int)na->y, mx, my);
            }
        }
        // ── Draw nodes ──
        for (auto &nd : nodes)
        {
            SDL_Color bc = (nd.comp >= 0) ? COMP_COLORS[nd.comp % NUM_CC] : SDL_Color{108, 113, 133, 255};
            int cx2 = (int)nd.x, cy2 = (int)nd.y;
            if (nd.isAP)
            {
                disk(ren, cx2, cy2, NODE_RADIUS + 7, {255, 68, 92, 38});
                for (int t = 0; t < 3; t++)
                    circle(ren, cx2, cy2, NODE_RADIUS + 4 + t, COL_ARTPT);
            }
            if (isDark)
                disk(ren, cx2 + 2, cy2 + 3, NODE_RADIUS, {0, 0, 0, 60});
            disk(ren, cx2, cy2, NODE_RADIUS, bc);
            if (nd.id == edgeStart)
                for (int t = 0; t < 3; t++)
                    circle(ren, cx2, cy2, NODE_RADIUS + 1 + t, th.accent);
            circle(ren, cx2, cy2, NODE_RADIUS, {255, 255, 255, isDark ? (Uint8)40 : (Uint8)65});
            SDL_Color lc = nd.isAP ? SDL_Color{255, 255, 255, 255} : SDL_Color{10, 10, 20, 255};
            text(ren, fNd, std::to_string(nd.id), cx2, cy2, lc, true);
        }

        // ════════════════════════════════════════════════════════════════════
        // BOTTOM PANELS
        // ════════════════════════════════════════════════════════════════════

        // Panel background
        fr(ren, SIDEBAR_W, BP_Y, WINDOW_W - SIDEBAR_W, BOTTOM_H, th.panel2);
        dl(ren, SIDEBAR_W, BP_Y, WINDOW_W, BP_Y, th.border);

        // Dividers between panels
        dl(ren, BP2_X, BP_Y, BP2_X, WINDOW_H, th.border);
        dl(ren, BP3_X, BP_Y, BP3_X, WINDOW_H, th.border);

        // Panel inner padding
        const int PP = 8; // padding
        const int PT = 6; // top padding from BP_Y

        // ────────────────────────────────────────────────────────────────────
        // PANEL 1: Thanh phan lien thong
        // ────────────────────────────────────────────────────────────────────
        {
            int px = BP1_X, pw = BP_W;
            text(ren, fMd, "THANH PHAN LIEN THONG", px + PP, BP_Y + PT, th.textDim);
            dl(ren, px + PP, BP_Y + PT + 18, px + pw - PP, BP_Y + PT + 18, th.border);

            // Build component -> nodes map
            std::map<int, std::vector<int>> compNodes;
            for (auto &nd : nodes)
                compNodes[nd.comp].push_back(nd.id);

            int rowY = BP_Y + PT + 20;
            int rowH = 22;
            for (auto &[cid, nids] : compNodes)
            {
                if (rowY + rowH > WINDOW_H - 4)
                    break;
                SDL_Color cc = COMP_COLORS[cid % NUM_CC];
                // color swatch
                disk(ren, px + PP + 8, rowY + rowH / 2, 7, cc);
                // label
                std::string lbl = "TPLT " + (std::to_string(cid + 1)) + ": {";
                for (int i = 0; i < (int)nids.size(); i++)
                {
                    lbl += std::to_string(nids[i]);
                    if (i + 1 < (int)nids.size())
                        lbl += ",";
                }
                lbl += "}";
                // clip to panel width
                SDL_Rect clip = {px, BP_Y, pw, BOTTOM_H};
                textClip(ren, fSm, lbl, px + PP + 20, rowY + 3, th.text, clip);
                rowY += rowH;
            }
            if (nodes.empty())
            {
                text(ren, fSm, "(chua co dinh nao)", px + PP, BP_Y + PT + 22, th.textDim);
            }
        }

        // ────────────────────────────────────────────────────────────────────
        // PANEL 2: Danh sach canh (scrollable)
        // ────────────────────────────────────────────────────────────────────
        {
            int px = BP2_X, pw = BP_W;
            text(ren, fMd, "DANH SACH CANH", px + PP, BP_Y + PT, th.textDim);
            // bridge count badge
            if (brCnt > 0)
            {
                std::string bc2 = std::to_string(brCnt) + " cau";
                fr(ren, px + pw - PP - 52, BP_Y + PT - 1, 52, 15, {180, 100, 0, 255});
                text(ren, fSm, bc2, px + pw - PP - 50, BP_Y + PT, {255, 230, 100, 255});
            }
            dl(ren, px + PP, BP_Y + PT + 16, px + pw - PP, BP_Y + PT + 16, th.border);

            // Header row
            int hY = BP_Y + PT + 20;
            SDL_Color hc = th.textDim;
            text(ren, fMd, "Canh", px + PP, hY, hc);
            text(ren, fMd, "Loai", px + PP + 80, hY, hc);
            dl(ren, px + PP, hY + 18, px + pw - PP, hY + 18, th.border);

            // Scrollable area
            int listTop = hY + 16;
            int listH = WINDOW_H - listTop - 4;
            int rowH = ROW_H;
            int totalH = (int)edges.size() * rowH;
            // clamp scroll
            edgeScrollY = std::max(0, std::min(edgeScrollY, std::max(0, totalH - listH)));

            // clip
            SDL_Rect clipR = {px, listTop, pw, listH};
            SDL_RenderSetClipRect(ren, &clipR);

            for (int i = 0; i < (int)edges.size(); i++)
            {
                int ry = listTop + i * rowH - edgeScrollY;
                if (ry + rowH < listTop || ry > listTop + listH)
                    continue;
                auto &e = edges[i];
                bool isBr = e.isBridge;
                // highlight bridge rows
                if (isBr)
                {
                    SDL_Color rowBg = isDark ? COL_BRIDGE_ROW : COL_BRIDGE_ROW_L;
                    fr(ren, px + 1, ry, pw - 2, rowH - 1, rowBg);
                }
                // alternating row tint
                else if (i % 2 == 1)
                {
                    SDL_Color alt = isDark ? SDL_Color{255, 255, 255, 8} : SDL_Color{0, 0, 0, 8};
                    fr(ren, px + 1, ry, pw - 2, rowH - 1, alt);
                }
                SDL_Color tc2 = isBr ? COL_BRIDGE : th.text;
                std::string edgeLbl = "(" + std::to_string(e.u) + " - " + std::to_string(e.v) + ")";
                text(ren, fSm, edgeLbl, px + PP, ry + 2, tc2);
                if (isBr)
                {
                    // bridge badge
                    fr(ren, px + PP + 76, ry + 3, 36, 13, COL_BRIDGE);
                    text(ren, fSm, "CAU", px + PP + 78, ry + 3, {20, 10, 0, 255});
                }
                else
                {
                    text(ren, fSm, "thuong", px + PP + 80, ry + 2, th.textDim);
                }
            }
            SDL_RenderSetClipRect(ren, nullptr);

            // Scrollbar
            if (totalH > listH && totalH > 0)
            {
                int sbH = std::max(20, (int)((float)listH / totalH * listH));
                int sbY = listTop + (int)((float)edgeScrollY / totalH * listH);
                fr(ren, px + pw - 5, listTop, 4, listH, th.border);
                fr(ren, px + pw - 5, sbY, 4, sbH, th.accent);
            }
            if (edges.empty())
                text(ren, fSm, "(chua co canh nao)", px + PP, listTop + 2, th.textDim);
        }

        // ────────────────────────────────────────────────────────────────────
        // PANEL 3: Ma tran ke (scrollable)
        // ────────────────────────────────────────────────────────────────────
        {
            int px = BP3_X, pw = WINDOW_W - BP3_X;
            text(ren, fMd, "MA TRAN KE", px + PP, BP_Y + PT, th.textDim);
            dl(ren, px + PP, BP_Y + PT + 18, px + pw - PP, BP_Y + PT + 18, th.border);

            int matTop = BP_Y + PT + 20;
            int matH = WINDOW_H - matTop - 4;
            int matW = pw - PP * 2;

            int n = (int)nodes.size();
            if (n == 0)
            {
                text(ren, fSm, "(chua co dinh nao)", px + PP, matTop + 2, th.textDim);
            }
            else
            {
                // Build adjacency + bridge map
                std::map<int, int> idxOf;
                for (int i = 0; i < n; i++)
                    idxOf[nodes[i].id] = i;
                std::set<std::pair<int, int>> bridgeSet;
                for (auto &e : edges)
                    if (e.isBridge)
                    {
                        int ui = idxOf.count(e.u) ? idxOf[e.u] : -1;
                        int vi = idxOf.count(e.v) ? idxOf[e.v] : -1;
                        if (ui >= 0 && vi >= 0)
                        {
                            bridgeSet.insert({ui, vi});
                            bridgeSet.insert({vi, ui});
                        }
                    }

                int cell = MAT_CELL;
                int headerW = cell; // row header col
                int totalMatW = headerW + n * cell;
                int totalMatH = cell + n * cell; // col header + rows

                // clamp scroll
                matScrollX = std::max(0, std::min(matScrollX, std::max(0, totalMatW - matW)));
                matScrollY = std::max(0, std::min(matScrollY, std::max(0, totalMatH - matH)));

                SDL_Rect clipMat = {px + PP, matTop, matW, matH};
                SDL_RenderSetClipRect(ren, &clipMat);

                int ox = px + PP - matScrollX;
                int oy = matTop - matScrollY;

                // Column headers (node IDs)
                for (int j = 0; j < n; j++)
                {
                    int cx2 = ox + headerW + j * cell;
                    int cy2 = oy;
                    if (cx2 + cell < px + PP || cx2 > px + PP + matW)
                        continue;
                    SDL_Color hc2 = nodes[j].isAP ? COL_ARTPT : th.textDim;
                    fr(ren, cx2, cy2, cell - 1, cell - 1, isDark ? SDL_Color{30, 35, 55, 255} : SDL_Color{220, 225, 240, 255});
                    text(ren, fMono, std::to_string(nodes[j].id), cx2 + cell / 2, cy2 + cell / 2, hc2, true);
                }

                // Rows
                for (int i = 0; i < n; i++)
                {
                    int ry = oy + cell + i * cell;
                    if (ry + cell < matTop || ry > matTop + matH)
                        continue;

                    // Row header
                    SDL_Color rhc = nodes[i].isAP ? COL_ARTPT : th.textDim;
                    fr(ren, ox, ry, headerW - 1, cell - 1, isDark ? SDL_Color{30, 35, 55, 255} : SDL_Color{220, 225, 240, 255});
                    text(ren, fMono, std::to_string(nodes[i].id), ox + headerW / 2, ry + cell / 2, rhc, true);

                    for (int j = 0; j < n; j++)
                    {
                        int cx2 = ox + headerW + j * cell;
                        if (cx2 + cell < px + PP || cx2 > px + PP + matW)
                            continue;

                        // Check adjacency
                        bool adj = false;
                        for (auto &e : edges)
                            if ((e.u == nodes[i].id && e.v == nodes[j].id) || (e.u == nodes[j].id && e.v == nodes[i].id))
                            {
                                adj = true;
                                break;
                            }

                        bool isBr = adj && bridgeSet.count({i, j}) > 0;
                        bool isDiag = (i == j);

                        SDL_Color cellBg;
                        if (isDiag)
                            cellBg = isDark ? SDL_Color{35, 40, 62, 255} : SDL_Color{210, 215, 235, 255};
                        else if (isBr)
                            cellBg = isDark ? SDL_Color{80, 55, 10, 255} : SDL_Color{255, 235, 180, 255};
                        else if (adj)
                            cellBg = isDark ? SDL_Color{30, 55, 90, 255} : SDL_Color{200, 220, 255, 255};
                        else
                            cellBg = isDark ? SDL_Color{18, 21, 33, 255} : SDL_Color{235, 238, 248, 255};

                        fr(ren, cx2, ry, cell - 1, cell - 1, cellBg);

                        SDL_Color tc2;
                        std::string val;
                        if (isDiag)
                        {
                            val = "0";
                            tc2 = th.textDim;
                        }
                        else if (isBr)
                        {
                            val = "1";
                            tc2 = COL_BRIDGE;
                        }
                        else if (adj)
                        {
                            val = "1";
                            tc2 = th.accent;
                        }
                        else
                        {
                            val = "0";
                            tc2 = th.textDim;
                        }

                        text(ren, fMono, val, cx2 + cell / 2, ry + cell / 2, tc2, true);
                    }
                }

                SDL_RenderSetClipRect(ren, nullptr);

                // Scrollbars
                // Horizontal
                if (totalMatW > matW)
                {
                    int sbW = std::max(20, (int)((float)matW / totalMatW * matW));
                    int sbX = px + PP + (int)((float)matScrollX / totalMatW * matW);
                    fr(ren, px + PP, WINDOW_H - 5, matW, 4, th.border);
                    fr(ren, sbX, WINDOW_H - 5, sbW, 4, th.accent);
                }
                // Vertical
                if (totalMatH > matH)
                {
                    int sbH2 = std::max(20, (int)((float)matH / totalMatH * matH));
                    int sbY2 = matTop + (int)((float)matScrollY / totalMatH * matH);
                    fr(ren, px + pw - 5, matTop, 4, matH, th.border);
                    fr(ren, px + pw - 5, sbY2, 4, sbH2, th.accent);
                }

                // Legend for matrix
                if (n <= 6)
                {
                    // show color legend small
                }
            }
        }

        // Bottom border line (top of bottom panels)
        dl(ren, SIDEBAR_W, BP_Y, WINDOW_W, BP_Y, th.border);

        SDL_RenderPresent(ren);
        SDL_Delay(8);
    }

    TTF_CloseFont(fNd);
    TTF_CloseFont(fSm);
    TTF_CloseFont(fMd);
    TTF_CloseFont(fLg);
    TTF_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}