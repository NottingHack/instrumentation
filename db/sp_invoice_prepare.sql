drop procedure if exists sp_invoice_prepare;

/*
  Creates entries in invoices table for each member that should be sent
  and invoice for the selected invoice period.
  
  invoice_month & invoice_year specify which month to generate invoices for.
  Either both or neither must be supplied (if both null, defaults to last month).
  
  member_id can be specified to generate an invoice for one member only.    

 */

DELIMITER //
CREATE PROCEDURE sp_invoice_prepare
(
   IN  invoice_year   int,
   IN  invoice_month  int, 
   IN  member_id      int,
   OUT err            varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  
  declare v_invoice_from  date; -- inclusive
  declare v_invoice_to    date; -- exclusive
  
  set err = '';

  main: begin  

    -- Validate input params
    if 
    (
      (
        (invoice_year is null) and
        (invoice_month is not null)
      ) or
      (
        (invoice_month is null) and
        (invoice_year is not null)
      )
    ) then
      set err = 'Either both invoice_month and invoice_year must be specified, or neither';
      leave main;
    end if;
    
    -- Set invoice to/from dates.
    if (invoice_year is not null) then
      set v_invoice_from = str_to_date(concat('1,', invoice_month, ',', invoice_year),'%d,%m,%Y');
      set v_invoice_to = date_add(v_invoice_from, interval 1 month);
    else -- invoice period not specified; use previous month
      -- First date of the current month to the first of the month before.
      set v_invoice_to =  str_to_date(concat('1,', (select extract(month from now())), ',', (select extract(year from now()))),'%d,%m,%Y');
      set v_invoice_from = date_sub(v_invoice_to, interval 1 month);
    end if;
    
    -- Sanity check to date
    if (v_invoice_to > now()) then
      set err = 'Error: Invoice period extends past current date!';
      leave main;
    end if;
      
    -- Insert an invoice record for each member that either has a non-zero balance, 
    -- or has transactions in the invoice period 
    insert into invoices (member_id, invoice_from, invoice_to, invoice_generated, invoice_status, invoice_amount)
    select
      m.member_id,
      v_invoice_from,
      v_invoice_to,
      now(),
      'GENERATING',
      m.balance
    from members m
    where 
    (
      exists
      (
        -- Member has transactions in the invoice month
        select null
        from transactions t
        where t.transaction_datetime >= v_invoice_from
          and t.transaction_datetime <  v_invoice_to
          and t.member_id = m.member_id
      ) or 
      (
        -- member has a non-zero balance
        m.balance != 0
      )
    ) and not exists  -- check there isn't already an invoice for this period
    ( 
      select null
      from invoices i
      where i.member_id = m.member_id
        and i.invoice_from  = v_invoice_from
        and i.invoice_to    = v_invoice_to    
        and i.invoice_status in ('GENERATING','GENERATED')
        for update
    ) and 
    (
      m.member_id = member_id or member_id is null
    );
   
  end main;
 

END //
DELIMITER ;
