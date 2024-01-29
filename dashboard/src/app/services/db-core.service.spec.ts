import { TestBed } from '@angular/core/testing';

import { DbCoreService } from './db-core.service';

describe('DbCoreService', () => {
  let service: DbCoreService;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    service = TestBed.inject(DbCoreService);
  });

  it('should be created', () => {
    expect(service).toBeTruthy();
  });
});
