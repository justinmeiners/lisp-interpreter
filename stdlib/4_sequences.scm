(define (map proc . rest) 
 (define (helper lists result) 
  (if (some? null? lists) 
   (reverse! result) 
   (helper (map1 cdr lists) 
    (cons (apply proc (map1 car lists)) result)))) 
 (helper rest '())) 

(define (for-each proc . rest) 
 (define (helper lists) 
  (if (some? null? lists) 
   '() 
   (begin 
    (apply proc (map1 car lists)) 
    (helper (map1 cdr lists))))) 
 (helper rest)) 

(define (filter pred l) 
 (define (helper l result) 
  (cond ((null? l) result) 
   ((pred (car l)) 
    (helper (cdr l) (cons (car l) result))) 
   (else 
    (helper (cdr l) result)))) 
 (reverse! (helper l '()))) 


(define (alist->hash-table alist) 
  (define h (make-hash-table)) 
  (for-each1 (lambda (pair) 
               (hash-table-set! h (car pair) (cdr pair))) alist) 
  h) 

(define (_assoc key list eq?) 
 (if (null? list) #f 
  (let ((pair (car list))) 
    (if (and (pair? pair) (eq? key (car pair))) 
        pair 
        (_assoc key (cdr list) eq?))))) 

(define (assoc key list) (_assoc key list equal?)) 
(define (assq key list) (_assoc key list eq?)) 
(define (assv key list) (_assoc key list eqv?)) 

(define (_member x list eq?) 
 (cond ((null? list) #f) 
  ((eq? (car list) x) list) 
  (else (_member x (cdr list) eq?)))) 

(define (member x list) (_member x list equal?)) 
(define (memq x list) (_member x list eq?)) 
(define (memv x list) (_member x list eqv?)) 
 
(define (make-initialized-vector l fn) 
  (let ((v (make-vector l '()))) 
    (do ((i 0 (+ i 1))) 
      ((>= i l) v) 
      (vector-set! v i (fn i))))) 

(define (vector-map fn v) 
  (make-initialized-vector 
    (vector-length v) 
    (lambda (i) (fn (vector-ref v i))))) 

(define (vector-binary-search v key< unwrap-key key) 
  (define (helper low high mid) 
    (if (<= (- high low) 1) 
        (if (key< (unwrap-key (vector-ref v low)) key) #f (vector-ref v low)) 
        (begin 
          (set! mid (+ low (quotient (- high low) 2))) 
          (if (key< key (unwrap-key (vector-ref v mid))) 
              (helper low mid 0) 
              (helper mid high 0))))) 
  (helper 0 (vector-length v) 0)) 

(define (quicksort-partition v lo hi op) 
  (do ((pivot (vector-ref v (/ (+ lo hi) 2)) pivot) 
       (i (- lo 1) i) 
       (j (+ hi 1) j)) 
    ((>= i j) j) 
    (begin 
      (do ((x (set! i (+ i 1)) x)) 
        ((not (op (vector-ref v i) pivot)) '()) 
        (set! i (+ i 1))) 
      (do ((x (set! j (- j 1)) x)) 
        ((not (op pivot (vector-ref v j))) '()) 
        (set! j (- j 1))) 
      (if (< i j) (vector-swap! v i j))))) 

(define (quicksort-vector v lo hi op) 
  (if (and (>= lo 0) (>= hi 0) (< lo hi)) 
      (let ((p (quicksort-partition v lo hi op))) 
        (quicksort-vector v lo p op) 
        (quicksort-vector v (+ p 1) hi op)))) 

(define (sort! v op) 
  (quicksort-vector v 0 (- (vector-length v) 1) op) v) 

(define (sort list cmp) (vector->list (sort! (list->vector list) cmp))) 



