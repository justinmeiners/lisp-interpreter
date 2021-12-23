(define-macro lambda (/\_ args
           (if (pair? args)
               (if (pair? (cdr args))
                   (if (pair? (cdr (cdr args)))
                       `(/\_ ,(car args) ,(cons 'BEGIN (cdr args)))
                       `(/\_ ,(car args) ,(car (cdr args))))
                   (syntax-error "lambda missing body expressions: (lambda (args) body)"))
               (syntax-error "lambda missing argument: (lambda (args) body)"))))

(define-macro set! (lambda (var x)
                     (begin
                       (if (not (symbol? var)) (syntax-error "set! not a variable"))
                       `(_SET! ,var ,x))))

(define-macro define
              (lambda (var . exprs)
                (if (symbol? var)
                    (if (pair? (cdr exprs))
                        (syntax-error "define: (define var x)")
                        `(_DEF ,var ,(car exprs)))
                    (if (pair? var)
                        `(_DEF ,(car var)
                               (LAMBDA ,(cdr var)
                                       ,(if (null? (cdr exprs)) (car exprs) (cons 'BEGIN exprs))))
                        (syntax-error "define: not a symbol") ))))
 
(define (first x) (car x)) 
(define (second x) (car (cdr x))) 
(define (third x) (car (cdr (cdr x)))) 

(define (some? pred l) 
  (if (null? l) #f 
      (if (pred (car l)) #t 
          (some? pred (cdr l))))) 

(define (_map1-helper proc l result) 
  (if (null? l) 
      (reverse! result) 
      (_map1-helper proc 
                    (cdr l) 
                    (cons (proc (car l)) result)))) 

(define (map1 proc l) (_map1-helper proc l '())) 

(define (for-each1 proc l) 
  (if (null? l) '() 
      (begin (proc (car l)) (for-each1 proc (cdr l ))))) 
 
(define (reverse! l) (append-reverse! l '()))
(define (reverse l) (reverse! (list-copy l))) 

(define (last-pair x) 
 (if (pair? (cdr x)) 
  (last-pair (cdr x)) x)) 

(define (list-tail x k) 
 (if (zero? k) x 
  (list-tail (cdr x) (- k 1)))) 
 
(define (reduce op acc lst) 
    (if (null? lst) acc 
        (reduce op (op acc (car lst)) (cdr lst)))) 

(define (_expand-shorthand-body path) 
  (if (null? path) (cons 'pair '()) 
      (list (if (char=? (car path) #\A) 
            (cons 'CAR (_expand-shorthand-body (cdr path))))))) 

(define (_expand-shorthand text) 
  (cons 'DEFINE  (cons (list (string->symbol (string-append "C" text "R")) 'pair) 
        (_expand-shorthand-body (string->list text))))) 

(define-macro _shorthand-accessors (lambda args (cons 'BEGIN (map1 _expand-shorthand args)))) 

(define (vector . args) (list->vector args))

(define (vector-copy v) (subvector v 0 (vector-length v)))
(define (vector-head v end) (subvector v 0 end)) 
(define (vector-tail v start) (subvector v start (vector-length v))) 
 
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
 
