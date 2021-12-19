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

(define (_make-lambda args body) 
  (list 'LAMBDA args (if (null? (cdr body)) (car body) (cons 'BEGIN body)))) 

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

(define (_cond-check-clauses clauses) 
  (for-each1 (lambda (clause) 
               (if (not (pair? clause)) (syntax error "Invalid cond clause")) 
               (if (null? (cdr clause)) (syntax-error "cond clause missing expression"))) 
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


(define (_expand-mnemonic-body path) 
  (if (null? path) (cons 'pair '()) 
      (list (if (char=? (car path) #\A) 
            (cons 'CAR (_expand-mnemonic-body (cdr path))))))) 

(define (_expand-mnemonic text) 
  (cons 'DEFINE  (cons (list (string->symbol (string-append "C" text "R")) 'pair) 
        (_expand-mnemonic-body (string->list text))))) 

(define-macro _mnemonic-accessors (lambda args (cons 'BEGIN (map1 _expand-mnemonic args)))) 


