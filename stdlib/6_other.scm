(define (procedure? p) (or (compiled-procedure? p) (compound-procedure? p))) 
 
(define (newline) (write-char #\newline)) 

(define-macro assert (lambda (body) 
                       `(if ,body '() 
                            (begin 
                              (display (quote ,body)) 
                              (error " assert failed"))))) 

(define-macro ==>  (lambda (test expected) 
                `(assert (equal? ,test (quote ,expected))) ))  
