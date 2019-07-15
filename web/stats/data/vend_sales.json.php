<?php

include "dbaccess_header.php";

$sql = <<<SQL
  select 
    p.short_description as Item, 
    concat('£', cast((p.price/100) as decimal(20,2))) as Price,
    (select count(*) from transactions t where t.product_id = p.id) as Purchases,
    case when exists (select null from vending_locations vs where vs.product_id = p.id) then 'Yes' else 'No' end as Available
  from products p 
  order by short_description;
SQL;

print sql_to_tt_json(db_pdo_link(), $sql);

?>
