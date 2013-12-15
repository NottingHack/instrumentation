drop procedure if exists sp_vend_request;

/*
 * Respond to VREQ messages from vending machine. Check rfid & tran id match up with an entry in vend_log.
 * Check the members.balance+credit_limit covers the requested amout.
 * If allowed (result=1), also create a pending entry in the transactions table (as the 
 * vending machine should start vending on receipt of a successful result).
 * 
 * Note on err: This should be <= 16 characters as it sent to the LCD on failure
 */

DELIMITER //
CREATE PROCEDURE sp_vend_request
(
   IN  rfid_serial  varchar(50),
   IN  vend_tran_id varchar(6),
   IN  amount       int,
   OUT err          varchar(100),
   OUT result       int
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  declare r_state   int;
  declare member_id int;
  declare balance   int;
  declare climit    int;
  declare tran_id   int;
  
  set result = 0;
  set err = '';

  main: begin  

    -- Check the transaction id & serial id match up with an active vend entry
    select count(*) into ck_exists
    from vend_log v  
    where v.vend_tran_id = vend_tran_id
      and v.rfid_serial = rfid_serial
      and v.req_datetime is null 
      and v.cancelled_datetime is null
      and v.transaction_id is null;
      
    if (ck_exists = 0) then
      set err = 'VR01 failed'; -- unable to find matching entry in vend_log (BUG?) 
      leave main;
    end if;
    
    -- check member_id is still active, get balance and credit limit
    select 
      m.member_id,
      m.balance,
      m.credit_limit,
      r.state
    into
      member_id,
      balance,
      climit,
      r_state
    from members m 
    inner join rfid_tags r on r.member_id = m.member_id 
    where r.rfid_serial = rfid_serial
    order by state limit 1
    for update;

    if (r_state != 10) then -- STATE_ACTIVE
      set err = 'VR02 Not active'; -- 'RFID serial not active';
      update vend_log
      set req_datetime = sysdate(), denied_reason = err, amount_scaled = amount 
      where vend_log.vend_tran_id = vend_tran_id;      
      leave main;
    end if;
    
    -- check the member_id matches that in the vend_log
    select count(*) into ck_exists
    from vend_log v  
    where v.vend_tran_id = vend_tran_id
      and v.rfid_serial = rfid_serial
      and v.member_id = member_id;
      
    if (ck_exists != 1) then
      set err = 'VR03 int error '; -- Member ID / rfid / tran_id mismatch (BUG?)';
      update vend_log
      set req_datetime = sysdate(), denied_reason = err, amount_scaled = amount 
      where vend_log.vend_tran_id = vend_tran_id;      
      leave main;
    end if;    
    
    if (((balance - amount) < (-1*climit)) and (amount > 0)) then
      -- Insufficient credit 
      -- TODO: Check/sum pending transactions
      set err = 'out of credit';
      update vend_log
      set req_datetime = sysdate(), denied_reason = err, amount_scaled = amount 
      where vend_log.vend_tran_id = vend_tran_id;
      leave main;      
    end if;

    -- create in-progress transaction
    set tran_id = 0;
    call sp_transaction_log (member_id, (-1*amount), 'VEND', 'PENDING', 'Vend in progress', null, tran_id, err);
    
    if (err != '') then
      leave main;
    end if;
    
    update vend_log l
    set req_datetime = sysdate(), amount_scaled = amount, transaction_id = tran_id
    where l.vend_tran_id = vend_tran_id;    
    
    set result = 1;

  end main;
 

END //
DELIMITER ;
