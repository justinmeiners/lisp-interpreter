(define (char>=? a b) (not (char<? a b))) 
(define (char>? a b) (char<? b a)) 
(define (char<=? a b) (not (char<? b a))) 

(define (char-ci=? a b) (char=? (char-downcase a) (char-downcase b)))
(define (char-ci<? a b) (char<? (char-downcase a) (char-downcase b)))

(define (char-ci>=? a b) (not (char-ci<? a b))) 
(define (char-ci>? a b) (char-ci<? b a)) 
(define (char-ci<=? a b) (not (char-ci<? b a))) 

(define (char-lower-case? c)
    (and (>= (char->integer c) (char->integer #\a))
         (<= (char->integer c) (char->integer #\z))))

(define (char-upper-case? c)
    (and (>= (char->integer c) (char->integer #\A))
         (<= (char->integer c) (char->integer #\Z))))

(define (procedure? p) (or (compiled-procedure? p) (compound-procedure? p))) 

(define (current-input-port) _current-input-port)
(define (current-output-port) _current-output-port)

(define (read . args) 
  (_read (if (null? args)
           (current-input-port)
           (car args))))

(define (write obj . args) 
  (_write obj (if (null? args)
                (current-output-port)
                (car args))))

(define (display obj . args) 
  (_display obj (if (null? args)
                  (current-output-port)
                  (car args))))


(define (write-char obj . args) 
  (_write-char obj (if (null? args)
                     (current-output-port)
                     (car args))))

(define (flush-output-port . args) 
  (_flush-output-port (if (null? args)
                        (current-output-port)
                        (car args))))


(define (newline) (write-char #\newline)) 

(define-macro assert (lambda (body) 
                       `(if ,body '() 
                            (begin 
                              (display (quote ,body)) 
                              (error " assert failed"))))) 

(define-macro ==>  (lambda (test expected) 
                `(assert (equal? ,test (quote ,expected))) ))  


