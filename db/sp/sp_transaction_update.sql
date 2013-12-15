drop procedure if exists sp_transaction_update;

/*
 * Update a pending transaction in the transactions table. Don't allow update
 * if transaction state is already completed/aborted.
 * If updating to COMPLETED status, also update member.balance.
 */

DELIMITER //
CREATE PROCEDURE sp_transaction_update
(
  IN  tran_id     int,
  IN  tran_status varchar(  8),
  IN  tran_desc   varchar( 50),
  OUT err         varchar(100)   
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  declare amount int;
  declare memberid int;
  set err = '';
  
  if ((tran_desc is not null) and (length(tran_desc)=0)) then
    set tran_desc = null;
  end if;
  
  main: begin  
 
    -- If the transaction is already at status 'COMPLETE', don't allow it to be changed
    select count(*) into ck_exists
    from transactions t 
    where t.transaction_id = tran_id
      and t.transaction_status in ('COMPLETE', 'ABORTED');
      
    if (ck_exists > 0) then
      set err = 'Transaction is already complete or aborted - can''t amend!';
      leave main;
    end if;
    
    -- Check the transaction id can be found
    select count(*) into ck_exists
    from transactions t 
    where t.transaction_id = tran_id;
    
    if (ck_exists != 1) then
      set err = 'Unable to find transaction details';
      leave main;
    end if;    
    
    -- If setting the transaction to COMPLETE, update member balance as well.
    if (tran_status = 'COMPLETE') then
    begin
      declare EXIT HANDLER for SQLEXCEPTION, SQLWARNING
      begin
        set err = 'Error - transaction rollback!';
        rollback;
      end;
      
      start transaction;
      
      select t.amount, t.member_id
      into amount, memberid
      from transactions t
      where t.transaction_id = tran_id;

      update transactions t
      set 
        t.transaction_status = tran_status,         
        t.transaction_desc = ifnull(tran_desc, t.transaction_desc) -- if a new desc isn't given, keep the old one.
      where transaction_id = tran_id;
      
      update members m
      set m.balance = m.balance + amount
      where m.member_id = memberid;  
      
      commit;
    end;
    else
      -- Not updating to COMPLETE, so just update details, not balance.
      update transactions t
      set 
        t.transaction_status = tran_status, 
        t.transaction_desc = ifnull(tran_desc, t.transaction_desc) -- if a new desc isn't given, keep the old one.
      where transaction_id = tran_id;         
    end if;
    

  end main;

END //
DELIMITER ;
