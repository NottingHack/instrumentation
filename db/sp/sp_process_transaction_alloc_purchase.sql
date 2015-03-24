drop procedure if exists sp_process_transaction_alloc_purchase;

/* Allocate an existing purchase to the passed in payment */

DELIMITER //
CREATE PROCEDURE sp_process_transaction_alloc_purchase
(
   IN  p_transaction_id int,
   IN  p_member_id      int,
   IN  p_amount         int,
   OUT p_err            varchar(100)
)
SQL SECURITY DEFINER
BEGIN

  declare v_transaction_id int;
  declare v_amount         int;
  declare v_amount_alloc   int;
  declare v_alloc          int;
  declare v_already_alloc  int;
  
  declare done int default false;
  
  declare unalloc_purchases_cur cursor for
      select 
        t.transaction_id, 
        -1*t.amount,
        ( -- amount of this purchase that's already been covered by a payment
          select ifnull(sum(pp.amount), 0)
          from purchase_payment pp
          where pp.transaction_id_purchase = t.transaction_id
        ) as already_alloc
      from transactions t
      where t.member_id = p_member_id
        and t.transaction_id < p_transaction_id
        and t.amount < 0
        and -1*t.amount >
        (
          select ifnull(sum(pp.amount), 0)
          from purchase_payment pp
          where pp.transaction_id_purchase = t.transaction_id
            and pp.transaction_id_purchase <= p_transaction_id
            and pp.transaction_id_payment  <= p_transaction_id
        )
        order by t.transaction_id;

  declare continue handler for not found set done = TRUE;
  
  set v_amount_alloc = 0;

  main: begin  

    open unalloc_purchases_cur;
    
    read_loop: LOOP
      fetch unalloc_purchases_cur into v_transaction_id, v_amount, v_already_alloc;
        
      
      if done then 
        leave read_loop;
      end if;

      if (v_amount_alloc >= p_amount) then
        leave read_loop;
      end if;
      
      set v_amount = v_amount - v_already_alloc; -- If part of the purchase has already been covered, don't double allocate payment
      
      if (v_amount > (p_amount-v_amount_alloc)) then set v_alloc = (p_amount-v_amount_alloc);
      else
        set v_alloc = v_amount;
      end if;
      

      
      insert into purchase_payment (transaction_id_purchase, transaction_id_payment, amount )
      values                       ( v_transaction_id      , p_transaction_id      , v_alloc);
    
      set v_amount_alloc = v_amount_alloc + v_alloc;

    end loop; 

  end main;
  

END //
DELIMITER ;
