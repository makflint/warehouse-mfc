/* warehouse-mfc — demo seed data
   Run after 01_schema.sql:
   sqlcmd -S "(localdb)\MSSQLLocalDB" -i db\02_seed.sql                    */

USE Warehouse;
GO

DELETE FROM StockMovements;
DELETE FROM Products;
DELETE FROM Warehouses;
GO

INSERT INTO Warehouses (Code, Name) VALUES
    (N'MAG-A', N'Magazyn glowny A'),
    (N'MAG-B', N'Magazyn zapasowy B');

INSERT INTO Products (Sku, Name, Unit, ReorderLevel) VALUES
    (N'4521', N'Mlotek stalowy 500g',     N'szt', 10),
    (N'4522', N'Wkretarka akumulatorowa', N'szt',  5),
    (N'4523', N'Tasma miernicza 5m',      N'szt', 20),
    (N'4524', N'Rekawice robocze L',      N'par', 50),
    (N'4525', N'Sruba M8x40 (100szt)',    N'opak',15);
GO

/* Seed some opening stock via the stored proc so the transaction path is exercised.
   Resolve IDs by natural key (Sku/Code) instead of hardcoding IDENTITY values, so the
   seed stays correct even when re-run without recreating the schema (DELETE does not
   reset IDENTITY, so the IDs are not guaranteed to be 1..N on a second run). */
DECLARE @wA INT = (SELECT WarehouseId FROM Warehouses WHERE Code = N'MAG-A');
DECLARE @wB INT = (SELECT WarehouseId FROM Warehouses WHERE Code = N'MAG-B');
DECLARE @p4521 INT = (SELECT ProductId FROM Products WHERE Sku = N'4521');
DECLARE @p4522 INT = (SELECT ProductId FROM Products WHERE Sku = N'4522');
DECLARE @p4523 INT = (SELECT ProductId FROM Products WHERE Sku = N'4523');
DECLARE @p4524 INT = (SELECT ProductId FROM Products WHERE Sku = N'4524');
DECLARE @p4525 INT = (SELECT ProductId FROM Products WHERE Sku = N'4525');

DECLARE @n INT;
EXEC sp_RecordMovement @ProductId=@p4521, @WarehouseId=@wA, @Qty=40, @MovementType=N'IN', @Note=N'Stan poczatkowy', @NewOnHand=@n OUTPUT;
EXEC sp_RecordMovement @ProductId=@p4522, @WarehouseId=@wA, @Qty=8,  @MovementType=N'IN', @Note=N'Stan poczatkowy', @NewOnHand=@n OUTPUT;
EXEC sp_RecordMovement @ProductId=@p4523, @WarehouseId=@wA, @Qty=60, @MovementType=N'IN', @Note=N'Stan poczatkowy', @NewOnHand=@n OUTPUT;
EXEC sp_RecordMovement @ProductId=@p4524, @WarehouseId=@wA, @Qty=30, @MovementType=N'IN', @Note=N'Stan poczatkowy', @NewOnHand=@n OUTPUT;  -- below reorder (low)
EXEC sp_RecordMovement @ProductId=@p4525, @WarehouseId=@wB, @Qty=12, @MovementType=N'IN', @Note=N'Stan poczatkowy', @NewOnHand=@n OUTPUT;  -- below reorder (low)
EXEC sp_RecordMovement @ProductId=@p4521, @WarehouseId=@wA, @Qty=5,  @MovementType=N'OUT',@Note=N'Wydanie na produkcje', @NewOnHand=@n OUTPUT;
GO

SELECT * FROM vCurrentStock ORDER BY WarehouseCode, Sku;
GO
