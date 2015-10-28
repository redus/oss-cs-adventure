;; The first three lines of this file were inserted by DrRacket. They record metadata
;; about the language level of this file in a form that our tools can easily process.
#reader(lib "htdp-beginner-reader.ss" "lang")((modname larger) (read-case-sensitive #t) (teachpacks ()) (htdp-settings #(#t constructor repeating-decimal #f #t none #f () #f)))
(require 2htdp/image)

;; Image -> Boolean
;; return true if the first image's area is larger than the second image, false otherwise.
;; limit for rectangles only b/c there's no built-in to check image is rectangle/circle/etc.

; tests
(check-expect (larger? (rectangle 4 3 "solid" "blue") (rectangle 5 10 "solid" "blue"))
              false)
(check-expect (larger? (rectangle 4 3 "solid" "blue") (rectangle 4 10 "solid" "blue"))
              false)
(check-expect (larger? (rectangle 4 3 "solid" "blue") (rectangle 1 10 "solid" "blue"))
              true)
(check-expect (larger? (rectangle 4 3 "solid" "blue") (rectangle 7 3 "solid" "blue"))
              false)
(check-expect (larger? (rectangle 4 3 "solid" "blue") (rectangle 4 3 "solid" "blue"))
              false)
(check-expect (larger? (rectangle 4 3 "solid" "blue") (rectangle 2 3 "solid" "blue"))
              true)
(check-expect (larger? (rectangle 4 3 "solid" "blue") (rectangle 20 1 "solid" "blue"))
              false)
(check-expect (larger? (rectangle 4 3 "solid" "blue") (rectangle 4 1 "solid" "blue"))
              true)
(check-expect (larger? (rectangle 4 3 "solid" "blue") (rectangle 1 1 "solid" "blue"))
              true)

;(define (larger? img_0 img_1) true)
(define (larger? img0 img1)
  (> (area img0) (area img1)))

;; Image -> Number
;; return area of the image
(define (area img) (* (image-width img)(image-height img)))
