<?php

include "dbaccess_header.php";

$sql = <<<SQL
  select
    vm.description as VendingMachine,
    lv.name as Position,
    p.short_description as Product,
    p.long_description as Description,
    concat('£', cast((p.price/100) as decimal(20,2))) as Cost
  from vending_locations lv
  inner join vending_machines vm on vm.id = lv.vending_machine_id
  left outer join products p on p.id = lv.product_id
  order by vm.id, lv.encoding;
SQL;

print sql_to_tt_json(db_pdo_link(), $sql);

?>
