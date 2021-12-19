
(define (vector . args) (list->vector args))

(define (newline) (write-char #\newline)) 

(define (char>=? a b) (not (char<? a b))) 
(define (char>? a b) (char<? b a)) 
(define (char<=? a b) (not (char<? b a))) 

(define (string . chars) (list->string chars)) 

(define (string>=? a b) (not (string<? a b))) 
(define (string>? a b) (string<? b a)) 
(define (string<=? a b) (not (string<? b a))) 

(define (string-copy s) (substring s 0 (string-length v)))
(define (string-head s end) (subvector s 0 end)) 
(define (string-tail s start) (subvector s start (string-length v))) 

(define (alist->hash-table alist) 
  (define h (make-hash-table)) 
  (for-each1 (lambda (pair) 
               (hash-table-set! h (car pair) (cdr pair))) alist) 
  h) 

(define (vector-copy v) (subvector v 0 (vector-length v)))
(define (vector-head v end) (subvector v 0 end)) 
(define (vector-tail v start) (subvector v start (vector-length v))) 

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

(define (procedure? p) (or (compiled-procedure? p) (compound-procedure? p))) 

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

(define-macro assert (lambda (body) 
                       `(if ,body '() 
                            (begin 
                              (display (quote ,body)) 
                              (error " assert failed"))))) 

(define-macro ==>  (lambda (test expected) 
                `(assert (equal? ,test (quote ,expected))) )) 
