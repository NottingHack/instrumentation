
drop table if exists purchase_payment;

create table purchase_payment 
(
  transaction_id_purchase int,
  transaction_id_payment  int,
  amount                  int,
  primary key (transaction_id_purchase, transaction_id_payment)
) ENGINE = InnoDB; 

