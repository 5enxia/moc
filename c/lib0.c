void printToken();
void error(const char *format, ...) {
    vfprintf(stderr, format, (char*)&format + sizeof(char*));
    printToken();
    exit(1);
}
// 漢字コードは Shift-JIS とする
int isJp(int c){ return (c>=0x81 && c<=0x9F) || (c>=0xE0 && c<=0xFC); } // 2byte文字の判定

enum {  // ノード種別(およびトークン種別)
    CINT=1024, CDBL, CSTR, CCHR, ID, SY, // トークン種別はここまで. ID, SYはトークン専用
    VAR, MEMBER, FCALL, CDEXPR, LOR, LAND, AMP, STAR, SIZEOF
};
char *NodeType[] = {    // ノード種別表記
    "int","double","string","char", "id", "sy", 
    "var",".","fcall","cdexpr","||","&&", "&","*","sizeof"
};

//--------------------------------------------------------------------------------------//
//                                   名前管理表                                         //
//--------------------------------------------------------------------------------------//
#define  NMSIZE  20000  // 名前管理表のサイズ
enum { GLOBAL = 0, LOCAL };
enum { NM_VAR = 1, NM_FUNC, NM_STRT, NM_MEMB };   // 名前の種別

typedef struct {
    int  type;          // 名前の種別
    int  dataType;      // データ型
    char *name;         // 変数名または関数名
    int  address;       // 相対アドレス
    int  size[8];       // 配列サイズ
    int  ptrs;          // []の数（配列次元数）＋ *（ポインタ）の個数
    int  cc;            // Calling Convention.  0: CDECL, 1: WINAPI
} Name;

Name Names[NMSIZE];     // 名前管理表.
int  nGlobal, nLocal;   // 次の位置、現在の登録数 nGlobal, nLocal-nGlobal

// 指定された名前表から指定された名前を探す
int searchName0(int nB, int type, char *name) {
    int nBgn = nB==GLOBAL ? 0 : nGlobal;
    int n = nB==GLOBAL ? nGlobal : nLocal;	// nLast
    while (--n >= nBgn) {	// 表の末尾から探す
        if (strcmp(Names[n].name,name)==0 && Names[n].type==type) return n;
    }
    return -1;          // 見つからなかった。
} 

// 関数または変数情報を名前管理表に登録する
Name *appendName(int nB, int type, char *dt, char *name, int addr) {
    Name *pNew, nm = { type, 0, name, addr }; // 第二要素は下でセット
    if (dt != NULL && (strcmp(dt,"void")==0 || strcmp(dt,"int")==0 ||
                       strcmp(dt,"double")==0 || strcmp(dt,"string")==0)) {
        nm.dataType = NMSIZE + *dt;     // プリミティブデータ型変数宣言
    } else if (type == NM_STRT) {       // 構造体定義
        nm.dataType = nGlobal;
    } else if (dt != NULL) {            // 構造体変数宣言
        nm.dataType = searchName0(GLOBAL, NM_STRT, dt);
        if (nm.dataType < 0) error("appendName: '%s' undefined", dt);
    } else {
        error("appendName: nB=%d, type=%d, dt=%s", nB, type, dt);
    }
    pNew = nB==GLOBAL ? &Names[nGlobal++] : &Names[nLocal++];
    if (nGlobal>=NMSIZE || nLocal>=NMSIZE) error("appendName: 配列オーバーフロー");
    *pNew = nm;         // memcpy(pNew, &nm, sizeof(Name)) と等価
    return pNew;        // 新しいエントリのアドレスを返す
}

Name *searchName(int nB, int type, char *name) {
    int n = searchName0(nB, type, name);
    if (n >= 0) return &Names[n];
    if (nB == GLOBAL) error("searchName: '%s' undeclared", name);
    return NULL;
}

// 構造体要素のオフセットを得る
int getOffset(int dataType, char *member) {
    int n = 0;
    while (n < nGlobal && Names[n].dataType != dataType) n++;   // 型名で探索
    n++;        // 型名行をスキップ
    while (n < nGlobal && Names[n].type==NM_MEMB && strcmp(member,Names[n].name) != 0)
        n++;    // 要素名で探索
    if (n >= nGlobal) error("getOffset: '%s' not found", member);
    return Names[n].address;    // 構造体要素のオフセット
}

//--------------------------------------------------------------------------------------//
//                                   抽象構文木                                         //
//--------------------------------------------------------------------------------------//
enum {  // 中間語命令コード＆ノード種別
    // オペランドなし
    ADD=1, SUB, MUL, DIV, MOD, EQ, NE, GT, LT, GE, LE, AND, OR, SHL, SHR, INV, NOT, 
    PREINC, PREDEC, POSTINC, POSTDEC, LOADV, PushArg, PushAX, PopCX, RET, LOADC,
    SEQ, SNE, SET, ADDSET, SUBSET, MULSET, DIVSET, STP, ENTRY,
    // １オペランド
    PUSHI, PUSHA, LOADLA, LOADGA, LOADI, LOADS, ADDI, SUBI, MULI, DIVI, SHLI, EQI,
    NEI, GTI, LTI, GEI, LEI, SETI, ADDSETI, SUBSETI, ADDSP, WADDSP, JUMP, BEQ0, BNE0,
    CALL, SYSCALL, I2D, D2I, FUNCTION, LABEL
};
#define DBL 128         // 実数命令コード＝128+整数命令コード

char *OPCODE[] = {      // 命令コード表記
    "",  "add", "sub", "mul", "div", "mod",   "eq", "ne", "gt", "lt", "ge", "le",  "and",
    "or", "shl", "shr", "inv", "not",  "preinc", "predec", "postinc", "postdec", "loadv",
    "pusharg", "pushax", "popcx", "ret", "loadc", "seq", "sne", "set", "addset", "subset",
    "mulset", "divset",  "stp",  "entry", "pushi", "pusha", "loadla", "loadga", "loadi",
    "loads",  "addi", "subi", "muli", "divi", "shli",  "eqi", "nei", "gti", "lti", "gei",
    "lei", "seti","addseti", "subseti", "addsp","waddsp", "jump", "beq0", "bne0", "call",
    "syscall", "i2d", "d2i", "function", "label"
};
typedef struct Node {   // 抽象構文木構造体
    int  type;
    int  val;		// 文字列アドレスを格納することもある
    int  nSub, size;    // nSub:子ノード数, size: 最大コード数(メモリが確保された値)
    struct Node *sub[0];
} Node;

Node *allocNode(int type, int val, int nSub, int size) {
    Node node = { type, val, nSub, size };
    Node *pNode = malloc(sizeof(Node) + size*sizeof(Node*));
    if (pNode == NULL) error("allocNode: メモリ確保に失敗した. size=%d", size);
    return memcpy(pNode, &node, sizeof(Node));
}

Node *genNode(int type, int val, int nSub, ...) {
    Node **pSub = (Node**)((void*)&nSub+sizeof(int)); // 第4引数以降を配列として捉える.
    Node *pNode = allocNode(type, val, nSub, nSub);
    if (nSub > 0) memcpy(pNode->sub, pSub, sizeof(Node*) * nSub);
    return pNode;
}

Node *addSubNode(Node *pNode, Node *pSub) {
    if (pNode->nSub == pNode->size) {
        pNode->size *= 2;
        pNode = realloc(pNode, sizeof(Node) + pNode->size * sizeof(Node*));
        if (pNode == NULL) error("realloc: メモリ確保に失敗した. size=%d", pNode->size);
    }
    pNode->sub[pNode->nSub++] = pSub;
    return pNode;
}

void freeNodes(Node *pNode) {
    int n = 0;
    while (n < pNode->nSub) freeNodes(pNode->sub[n++]);
    free(pNode);        // サブノード(子供)を先に解放し、最後に自分(親)を解放する
}

void printAST(Node *node) {
    int n = 0, type = node->type;
    printf(" (%s", type < CINT ? OPCODE[type] : NodeType[type-CINT]);
    if (type==MEMBER || type==VAR) printf(" %s", (char*)node->val);
    else if (type == CSTR) printf(" \"%s\"", (char*)node->val);
    else if (type == CCHR) printf(" '%c'", node->val);
    else if (type == FCALL) printf(" %s", (char*)node->val);
    else if (type == CINT) printf(" %d", node->val);
    else if (type == CDBL) printf(" %.1f…", *((double*)node->val));
    while (n < node->nSub) printAST(node->sub[n++]);
    printf(")");
}
