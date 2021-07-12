ENTRY 0		; main関数の入り口
FRAME 3		; ローカル変数a,b,cの領域を確保
PUSHI 10	; 10をpush
STOREL 0	; popしてローカル変数#0(a)にストア
PUSHI 3		; 3をpush
STOREL 1	; popしてローカル変数#1(ab)にストア
LOADL 0		; a(#0)の値 を push
LOADL 1		; b(#1)の値 を push
ADD		; a+bが計算され結果がstackのトップに置かれる
STOREL 2	; stackのトップの値をc(#2)にセット
LOADL 2
PRINTLN "c=%d"
RET		; 関数呼び出しから戻る