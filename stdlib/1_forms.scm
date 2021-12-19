(_mnemonic-accessors "AA" "DD" "AD" "DA" "AAA" "AAD" "ADA" "DAA" "ADD" "DAD" "DDA" "DDD") 

(define (_and-helper preds) 
  (cond ((null? preds) #t) 
        ((null? (cdr preds)) (car preds)) 
        (else 
          `(IF ,(car preds) ,(_and-helper (cdr preds)) #f)))) 
(define-macro and (lambda preds (_and-helper preds))) 

(define (_or-helper preds var) 
  (cond ((null? preds) #f) 
        ((null? (cdr preds)) (car preds)) 
        (else 
          `(BEGIN (SET! ,var ,(car preds)) 
                  (IF ,var ,var ,(_or-helper (cdr preds) var)))))) 

(define-macro or (lambda preds 
                   (let ((var (gensym))) 
                     `(LET ((,var '())) ,(_or-helper preds var))))) 

(define-macro case (lambda (key . clauses) 
                     (let ((expr (gensym))) 
                       `(let ((,expr ,key)) 
                          ,(cons 'COND (map1 (lambda (entry) 
                                               (cons (if (pair? (car entry)) 
                                                         `(memv ,expr (quote ,(car entry))) 
                                                         (car entry)) 
                                                     (cdr entry))) clauses)))))) 

(define-macro push 
              (lambda (v l) 
                `(begin (set! ,l (cons ,v ,l)) ,l))) 

(define-macro do 
              (lambda (vars loop-check . loops) 
                (let ( (names '()) (inits '()) (steps '()) (f (gensym)) ) 
                  (for-each1 (lambda (var) 
                               (push (car var) names) 
                               (set! var (cdr var)) 
                               (push (car var) inits) 
                               (set! var (cdr var)) 
                               (push (car var) steps)) vars) 
                  `((lambda () 
                      (define ,f (lambda ,names 
                                   (if ,(car loop-check) 
                                       ,(if (pair? (cdr loop-check)) (car (cdr loop-check)) '()) 
                                       ,(cons 'BEGIN (append loops (list (cons f steps)))) ))) 
                      ,(cons f inits) 
                      )) ))) 

(define-macro dotimes 
              (lambda (form body) 
                (apply (lambda (i n . result) 
                         `(do ((,i 0 (+ ,i 1))) 
                            ((>= ,i ,n) ,(if (null? result) result (car result)) ) 
                            ,body) 
                         ) form))) 



