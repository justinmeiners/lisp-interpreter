
(define-macro delay (lambda (expr)
                      `(make-promise ,(cons 'LAMBDA
                                            (cons '()
                                                  (cons expr '()))))))

(define (force promise)
  (if (not (promise-forced? promise))
      (_promise-store! promise ((_promise-procedure promise))))
  (promise-value promise))

(define-macro cons-stream (lambda (x expr) `(cons ,x (delay ,expr))))

(define (stream-car stream) (car stream))
(define (stream-cdr stream) (force (cdr stream)))

(define (stream-pair? x)
  (and (pair? x) (promise? (cdr x))))

(define (stream-null? stream) (null? stream))

(define (stream->list-helper stream result)
  (if (stream-null? stream)
      (reverse! result)
      (stream->list-helper 
        (force (cdr stream)) 
        (cons (car stream) result)))) 

(define (stream->list stream) 
  (stream->list-helper stream '())) 

(define (list->stream list) 
  (if (null? list) 
      '() 
      (cons-stream (car list) (list->stream (cdr list))))) 

(define (stream . args) (list->stream args)) 

(define (stream-head-helper stream k result) 
  (if (= k 0) 
      (reverse! result) 
      (stream-head-helper (force (cdr stream)) (- k 1) (cons (car stream) result)))) 

(define (stream-head stream k) 
  (stream-head-helper stream k '())) 

(define (stream-tail stream k) 
  (if (= k 0) 
      stream 
      (stream-tail (stream-cdr stream) (- k 1)))) 

(define (stream-filter pred stream) 
  (cond ((stream-null? stream) the-empty-stream) 
        ((pred (stream-car stream)) 
         (cons-stream (stream-car stream) 
                      (stream-filter pred 
                                     (stream-cdr stream)))) 
        (else (stream-filter pred (stream-cdr stream)))))
