CREATE TABLE "nodeparcels" (
  "nodeid" TEXT NOT NULL,
  "parcelid" INTEGER NOT NULL,
   UNIQUE ("nodeid") ON CONFLICT IGNORE
);
