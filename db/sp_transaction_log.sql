drop procedure if exists sp_transaction_log;

/*
 * Log a transaction in transactions table. If tran_status is "COMPLETE", also update members.balance.
 */

DELIMITER //
CREATE PROCEDURE sp_transaction_log
(
   IN  member_id    int,
   IN  amount       int,
   IN  tran_type    varchar(6),
   IN  tran_status  varchar(8),
   IN  tran_desc    varchar(50),
   IN  recorded_by  int, 
   OUT tran_id      int,  
   OUT err          varchar(100)   
)
SQL SECURITY DEFINER
BEGIN
  declare member_exists int;
  set member_exists = 0;
  set err = '';

  main: begin  
 
    declare EXIT HANDLER for SQLEXCEPTION, SQLWARNING
    begin
      set err = 'Error - transaction rollback!';
      rollback;
    end;  

    -- check member_id is valid
    select count(*) into member_exists
    from members m
    where m.member_id = member_id;
  
    if (member_exists != 1) then
      set err = "Invalid member_id";  
      leave main;
    end if;
    
    if (tran_status not in ('COMPLETE', 'PENDING')) then
      set err = 'Error - invalid status';
      leave main;
    end if;

    start transaction;
    
    insert into transactions (member_id, amount, transaction_type, transaction_status, transaction_desc, recorded_by)
    values (member_id, amount, tran_type, tran_status, tran_desc, recorded_by);
    
    set tran_id = last_insert_id();

    if (tran_status = 'COMPLETE') then
      update members m
      set m.balance = m.balance + amount
      where m.member_id = member_id;  
    end if;

    commit;
  end main;

END //
DELIMITER ;
