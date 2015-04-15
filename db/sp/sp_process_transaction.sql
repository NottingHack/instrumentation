drop procedure if exists sp_process_transaction;

/*
 * The idea here is track what a payments relates to (i.e. snackspace or laser usage at the moment)
 * E.g.:
 * 1) £8 laser usage
 * 2) £5 snackspace/vend
 * 3) £10 payment
 * When the £10 payment is processed, £8 would be marked for laser use, £2 for snackspace.
 * This allocation of payments is stored in purchase_payment. If a member _isn't_ in credit,
 * then the sum of the amounts relating to them in purchase_payment should be equal to their
 * total payments.
 * If they are in credit, the difference between total payments, and the amount in 
 * purchase_payment will be the amount they are in credit by.
 */

DELIMITER //
CREATE PROCEDURE sp_process_transaction
(
   IN  p_transaction_id int,
   OUT p_err            varchar(100)
)
SQL SECURITY DEFINER
BEGIN

  declare v_member_id         int;
  declare v_amount            int;
  declare v_transaction_type  varchar(6);
  declare v_unalloc_payments  int;
  declare v_already_alloc     int;
  declare v_unalloc_purchase  int;


  main: begin  

  
    select t.member_id, t.amount, t.transaction_type
    into   v_member_id, v_amount, v_transaction_type
    from transactions t
    where t.transaction_id = p_transaction_id;
    
    if (v_amount = 0) then
      leave main;
    end if;
    
    if (v_amount < 0) then -- a purchase
      -- Check if any (get amount) payments have already been allocated to this transaction
      select -1*(ifnull(sum(pp.amount),0))
      into v_already_alloc
      from purchase_payment pp
      where pp.transaction_id_purchase = p_transaction_id;
      
      if (v_already_alloc <= v_amount) then
        -- there has already been enough payments allocated to cover the purchase, so exit
        leave main;
      end if;
    
      -- if part of the purchase has already been covered, only try and allocate a payment for the part that hasn't been.
      set v_amount = v_amount - v_already_alloc;
      
      -- sum unallocated payments
      select ifnull(sum(t.amount), 0)
      into v_unalloc_payments
      from transactions t
      where t.member_id = v_member_id
        and t.transaction_id < p_transaction_id
        and t.amount > 0
        and t.amount >
        (
          select ifnull(sum(pp.amount), 0)
          from purchase_payment pp
          where pp.transaction_id_payment = t.transaction_id
            and pp.transaction_id_purchase < p_transaction_id
            and pp.transaction_id_payment  < p_transaction_id
        );
        
        if (v_unalloc_payments > 0) then
          call sp_process_transaction_alloc_payment(p_transaction_id, v_member_id, -1*v_amount, p_err);
        end if;
      
    else
      -- v_amount > 0 - i.e. payment
      
      -- Check if any this payment has already been fully allocated to purchases
      select ifnull(sum(pp.amount),0)
      into v_already_alloc
      from purchase_payment pp
      where pp.transaction_id_payment = p_transaction_id;
      
      if (v_already_alloc >= v_amount) then
        -- payment already fully allocated, so exit
        leave main;
      end if;
      
      
      -- if part of the payment has already been allocated, only try and allocate the part that hasn't been.
      set v_amount = v_amount - v_already_alloc;
      
      -- sum purchases which haven't been (fully) allocated payment
      select ifnull(sum(-1*t.amount), 0)
      into v_unalloc_purchase
      from transactions t
      where t.member_id = v_member_id
        and t.transaction_id < p_transaction_id
        and t.amount < 0
        and -1*t.amount >
        (
          select ifnull(sum(pp.amount), 0)
          from purchase_payment pp
          where pp.transaction_id_purchase = t.transaction_id
            and pp.transaction_id_purchase < p_transaction_id
            and pp.transaction_id_payment  < p_transaction_id
        );
      
      if (v_unalloc_purchase > 0) then
        call sp_process_transaction_alloc_purchase(p_transaction_id, v_member_id, v_amount, p_err);
      end if;
      
      leave main;
    end if;
    
    

  end main;
  

END //
DELIMITER ;
