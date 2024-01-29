import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { YesnoComponent } from './yesno.component';

describe('YesnoComponent', () => {
  let component: YesnoComponent;
  let fixture: ComponentFixture<YesnoComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ YesnoComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(YesnoComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
