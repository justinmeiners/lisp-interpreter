(define (_make-lambda args body) 
  (list 'LAMBDA args (if (null? (cdr body)) (car body) (cons 'BEGIN body)))) 
 

; (LET <name> ((<var0> <expr0>) ... (<varN> <expr1>)) <body0> ... <bodyN>)
;  => ((LAMBDA (<var0> ... <varN>) (BEGIN <body0> ... <bodyN>)) <expr0> ... <expr1>)            
;  => named 
;    ((lambda ()
;        (define <name> (LAMBDA (<var0> ... <varN>) (BEGIN <body0> ... <bodyN>)))
;        (<name> <expr0> ... <exprN>)))

(define (_check-binding-list bindings) 
  (for-each1 (lambda (entry) 
               (if (not (pair? entry)) (syntax-error "bad let binding" entry)) 
               (if (not (symbol? (first entry))) (syntax-error "let entry missing symbol" entry))) bindings)) 
 
(define (_let->combination var bindings body) 
  (_check-binding-list bindings) 
  (define body-func (_make-lambda (map1 (lambda (entry) (first entry)) bindings) body)) 
  (define initial-args (map1 (lambda (entry) (second entry)) bindings)) 
  (if (null? var) 
      (cons body-func initial-args) 
      (list (_make-lambda '() (list (list 'DEFINE var body-func) (cons var initial-args)))))) 

(define-macro let (lambda args  
                    (if (pair? (first args)) 
                        (_let->combination '() (car args) (cdr args)) 
                        (_let->combination (first args) (second args) (cdr (cdr args)))))) 

(define (_let*-helper bindings body) 
  (if (null? bindings) (if (null? (cdr body)) (car body) (cons 'BEGIN body)) 
      (list 'LET (list (car bindings)) (_let*-helper (cdr bindings) body)))) 

(define-macro let* (lambda (bindings . body) 
                     (_check-binding-list bindings) 
                     (_let*-helper bindings body))) 

(define-macro letrec (lambda (bindings . body) 
                       (_check-binding-list bindings) 
                       (cons (_make-lambda (map1 (lambda (entry) (first entry)) bindings) 
                                           (append (map1 (lambda (entry) (list 'SET! (first entry) (second entry))) 
                                                         bindings) body)) 
                             (map1 (lambda (entry) '()) bindings)))) 


; (COND (<pred0> <expr0>)
;       (<pred1> <expr1>)
;        ...
;        (else <expr-1>)) 
; =>
; (IF <pred0> <expr0>
;      (if <pred1> <expr1>
;          ....
;      (if <predN> <exprN> <expr-1>)) ... )


(define (_cond-check-clauses clauses) 
  (for-each1 (lambda (clause) 
               (if (not (pair? clause)) (syntax-error "cond: invalid clause")) 
               (if (null? (cdr clause)) (syntax-error "cond: clause missing expression"))) 
             clauses)) 

(define (_cond-helper clauses) 
  (if (null? clauses) '() 
      (if (eq? (car (car clauses)) 'ELSE) 
          (cons 'BEGIN (cdr (car clauses))) 
          (list 'IF 
                (car (car clauses)) 
                (cons 'BEGIN (cdr (car clauses))) 
                (_cond-helper (cdr clauses)))))) 

(define-macro cond (lambda clauses 
                     (begin 
                       (_cond-check-clauses clauses) 
                       (_cond-helper clauses)))) 



