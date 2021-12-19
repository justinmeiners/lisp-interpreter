(define (append-reverse! l tail) 
  (if (null? l) tail 
    (let ((next (cdr l))) 
      (set-cdr! l tail) 
      (append-reverse! next l)))) 

(define (last-pair x) 
 (if (pair? (cdr x)) 
  (last-pair (cdr x)) x)) 

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

(define (list-tail x k) 
 (if (zero? k) x 
  (list-tail (cdr x) (- k 1)))) 

(define (filter pred l) 
 (define (helper l result) 
  (cond ((null? l) result) 
   ((pred (car l)) 
    (helper (cdr l) (cons (car l) result))) 
   (else 
    (helper (cdr l) result)))) 
 (reverse! (helper l '()))) 

(define (reduce op acc lst) 
    (if (null? lst) acc 
        (reduce op (op acc (car lst)) (cdr lst)))) 

(define (reverse l) (reverse! (list-copy l)))
