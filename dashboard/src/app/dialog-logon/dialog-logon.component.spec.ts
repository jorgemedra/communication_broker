import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { DialogLogonComponent } from './dialog-logon.component';

describe('DialogLogonComponent', () => {
  let component: DialogLogonComponent;
  let fixture: ComponentFixture<DialogLogonComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ DialogLogonComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(DialogLogonComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
