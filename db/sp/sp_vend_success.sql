drop procedure if exists sp_vend_success;

/*
  Vending machine has confirmed the vend was succesfull. Update vend_log with the datetime & item position,
  update the transactions table to show it's complete, and update the members balance.

*/

DELIMITER //
CREATE PROCEDURE sp_vend_success
(
   IN  rfid_serial  varchar( 50),
   IN  vend_tran_id varchar(  6),
   IN  pos          varchar( 10),
   OUT err          varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists  int;
  declare tran_id    int;
  declare vdesc      varchar(100);
  
  declare location   varchar(10);
  declare price      int;
  declare prddesc    varchar(25);
  declare prdid      int;
  declare vm_id      int;
  declare vm_type    varchar(10);

  set err = '';

  main: begin  

    -- Check the transaction id & serial id match up with an active vend entry
    select count(*) into ck_exists
    from vend_log v  
    where v.vend_tran_id = vend_tran_id
      and v.rfid_serial = rfid_serial 
      and v.cancelled_datetime is null
      and v.transaction_id is not null
      and v.position is null;
        
    if (ck_exists = 0) then
      set err = 'unable to find matching entry in vend_log (BUG)'; 
      leave main;
    end if;

    select 
      v.transaction_id,
      v.vmc_id,
      d.vmc_type
    into
      tran_id,
      vm_id,
      vm_type
    from vend_log v 
    inner join vmc_details d on d.vmc_id = v.vmc_id
    where v.vend_tran_id = vend_tran_id;

    if (vm_type = 'NOTE') then
      -- Payment made using note acceptor

      select concat('Note payment - £', format((-1*v.amount_scaled)/100, 0))
      into vdesc
      from vend_log v
      where v.vend_tran_id = vend_tran_id;
    else
      -- vending machine
      select count(*) into ck_exists
      from vmc_ref vr
      where vr.loc_encoded = pos
        and vr.vmc_id = vm_id;

      if (ck_exists = 0) then
        -- We don't recognise the locaation the vmc just vended from!
        -- Continue to record the vend, but log a warning in the events table
        call sp_log_event('WARN', 'VEND: Unknown VMC location reported');
        set location = 'Unknown';
        set prddesc = 'Unknown item';
      else
        -- Get product details - if known; but still record vend if not filled in.
        select vr.loc_name, p.product_id, p.price, coalesce(p.shortdesc, 'Unknown item') 
        into location, prdid, price, prddesc
        from vmc_ref vr
        left outer join vmc_state vs on vs.vmc_ref_id = vr.vmc_ref_id
        left outer join products p on vs.product_id = p.product_id
        where vr.loc_encoded = pos
          and vr.vmc_id = vm_id;
      end if;

      set vdesc = concat('[', prddesc, '] vended from location [', location, '].');
    end if; -- if vending machine

    call sp_transaction_update(tran_id, 'COMPLETE', vdesc, err);
    if (length(err) > 0) then
      leave main;
    end if;

    update transactions
    set product_id = prdid
    where transaction_id = tran_id;

    update vend_log
    set success_datetime = sysdate(), position = pos
    where vend_log.vend_tran_id = vend_tran_id;

  end main;


END //
DELIMITER ;

