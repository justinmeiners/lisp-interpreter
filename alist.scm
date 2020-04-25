
;(define (some? pred l)
;  (cond ((null? l) '())
;        ((pred (car l)) 1)
;        (else (some? pred (cdr l)))))

;(define (map1 proc l result)
;  (if (null? l)
;    (reverse! result)
;    (map1 proc
;          (cdr l)
;          (cons (proc (car l)) result))))

;(define (map proc . rest)
;  (define (helper lists result)
;    (if (some? null? lists)
;      (reverse! result)
;      (helper (map1 cdr lists '())
;              (cons (apply proc (map1 car lists '())) result))))
;  (reverse! (helper rest '())))

;(define (for-each proc . rest)
;  (define (helper lists)
;    (if (some? null? lists)
;      '()
;      (begin
;        (apply proc (map1 car lists '()))
;        (helper (map1 cdr lists '())))))
;  (helper rest))

;(define (alist->hash-table alist)
;  (define h (make-hash-table))
;  (for-each (lambda (pair)
;              (hash-table-set! h (car pair) (cdr pair)))
;            alist)
;  h)

(display (map (lambda (x) (* x x)) '(2 3 4)))
;(display (map (lambda (x y) (+ x y)) '(1 2 3) '(1 0 -1)))

(define h (alist->hash-table '((A . 3) (B . 4))))

(display h)
(display (hash-table->alist h))
